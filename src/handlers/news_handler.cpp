#include "news_handler.hpp"

#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/parser/parser.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>
#include <map>
#include "../gateway/redis_client.hpp"
#include "../gateway/http_client.hpp"
#include <sstream>

namespace news_aggregator::handlers {

std::string NewsHandler::HandleRequest(
    server::http::HttpRequest& request,
    server::request::RequestContext&) const {
  
  // Add CORS headers to allow browser access
  request.GetHttpResponse().SetHeader(std::string("Access-Control-Allow-Origin"), std::string("*"));
  request.GetHttpResponse().SetHeader(std::string("Access-Control-Allow-Methods"), std::string("GET, POST, OPTIONS"));
  request.GetHttpResponse().SetHeader(std::string("Access-Control-Allow-Headers"), std::string("Content-Type, Authorization"));
  
  
  request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);
  request.GetHttpResponse().SetHeader(http::headers::kCacheControl, "public, max-age=300");

          try {
            // Get limit parameter from query string
            int limit = 10;
            if (request.HasArg("limit")) {
              try {
                limit = std::stoi(request.GetArg("limit"));
                if (limit <= 0 || limit > 100) {
                  limit = 10; // Limit to reasonable values
                }
              } catch (const std::exception&) {
                limit = 10;
              }
            }

            // Create cache key
            std::stringstream cache_key_stream;
            cache_key_stream << "news:latest:" << limit;
            std::string cache_key = cache_key_stream.str();

            // Connect to Redis
            news_aggregator::gateway::RedisClient redis_client("localhost", 6379, 1000); // 1 second timeout
            bool redis_connected = redis_client.Connect();
            
            if (redis_connected) {
                // Check cache
                std::string cached_data = redis_client.GetJson(cache_key);
                if (!cached_data.empty()) {
                    request.GetHttpResponse().SetHeader(std::string("X-Cache"), std::string("HIT"));
                    
                    // Parse JSON and add cache_status
                    try {
                        auto cached_json = formats::json::FromString(cached_data);
                        formats::json::ValueBuilder builder(cached_json);
                        builder["cache_status"] = "HIT";
                        return formats::json::ToString(builder.ExtractValue());
                    } catch (const std::exception& ex) {
                        LOG_WARNING() << "Failed to parse cached JSON, returning as is: " << ex.what();
                        return cached_data;
                    }
                }
                
            } else {
                LOG_WARNING() << "Redis not available, proceeding without cache";
            }

            // Make HTTP request to StorageService
            news_aggregator::gateway::HttpClient http_client;
            http_client.SetUserAgent("API-Gateway/1.0");
            http_client.SetTimeout(10);
            
            std::string storage_url = "http://localhost:8080/news/latest?limit=" + std::to_string(limit);
            
            auto storage_response = http_client.Get(storage_url, 10);
            
            if (storage_response.success) {
                
                // Parse response from StorageService
                try {
                    // Clean HTML entities from JSON before parsing
                    std::string cleaned_json = storage_response.body;
                    
                    // Replace common HTML entities that can break JSON parsing
                    std::map<std::string, std::string> html_entities = {
                        {"&#8217;", "'"},
                        {"&#8216;", "'"},
                        {"&#8220;", "\""},
                        {"&#8221;", "\""},
                        {"&#8211;", "–"},
                        {"&#8212;", "—"},
                        {"&#8230;", "…"},
                        {"&amp;", "&"},
                        {"&lt;", "<"},
                        {"&gt;", ">"},
                        {"&quot;", "\""},
                        {"&#39;", "'"}
                    };
                    
                    for (const auto& entity : html_entities) {
                        size_t pos = 0;
                        while ((pos = cleaned_json.find(entity.first, pos)) != std::string::npos) {
                            cleaned_json.replace(pos, entity.first.length(), entity.second);
                            pos += entity.second.length();
                        }
                    }
                    
                    auto storage_json = formats::json::FromString(cleaned_json);
                    
                    // Add cache information
                    formats::json::ValueBuilder response_builder(storage_json);
                    response_builder["cache_status"] = redis_connected ? "MISS" : "DISABLED";
                    response_builder["gateway_info"] = "API Gateway successfully integrated with StorageService";
                    
                    // Save to cache if Redis is available
                    if (redis_connected) {
                        redis_client.SetJson(cache_key, formats::json::ToString(response_builder.ExtractValue()), 300); // 5 minutes TTL
                    }
                    
                    return formats::json::ToString(response_builder.ExtractValue());
                    
                } catch (const std::exception& ex) {
                    LOG_ERROR() << "Failed to parse StorageService response: " << ex.what();
                    // Fallback to error response
                    formats::json::ValueBuilder error_response;
                    error_response["status"] = "error";
                    error_response["message"] = "Failed to parse StorageService response";
                    error_response["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    error_response["cache_status"] = "ERROR";
                    error_response["news"] = formats::json::ValueBuilder{}.ExtractValue(); // Empty array
                    
                    request.GetHttpResponse().SetHeader(std::string("X-Cache"), std::string("ERROR"));
                    return formats::json::ToString(error_response.ExtractValue());
                }
            } else {
                LOG_ERROR() << "Failed to get response from StorageService, status: " << storage_response.status_code;
            }
            
            // Fallback: return error if failed to get data from StorageService
            formats::json::ValueBuilder error_response;
            error_response["status"] = "error";
            error_response["message"] = "Failed to fetch news from StorageService";
            error_response["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            error_response["cache_status"] = "ERROR";
            error_response["news"] = formats::json::ValueBuilder{}.ExtractValue(); // Empty array

            request.GetHttpResponse().SetHeader(std::string("X-Cache"), std::string("ERROR"));
            return formats::json::ToString(error_response.ExtractValue());

  } catch (const std::exception& ex) {
    LOG_ERROR() << "Error in NewsHandler: " << ex.what();
    
    formats::json::ValueBuilder error_response;
    error_response["status"] = "error";
    error_response["message"] = "Internal server error";
    error_response["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    request.GetHttpResponse().SetStatus(server::http::HttpStatus::kInternalServerError);
    return formats::json::ToString(error_response.ExtractValue());
  }
}

}  // namespace news_aggregator::handlers