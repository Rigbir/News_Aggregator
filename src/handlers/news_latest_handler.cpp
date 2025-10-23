#include "news_latest_handler.hpp"

#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>
#include "../storage/postgres_client.hpp"

namespace news_aggregator::handlers {

std::string NewsLatestHandler::HandleRequest(
    server::http::HttpRequest& request,
    server::request::RequestContext&) const {
  
  request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);
  request.GetHttpResponse().SetHeader(http::headers::kCacheControl, "public, max-age=60");

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

    // Create PostgreSQL client
    std::string connection_string = "host=localhost port=5432 dbname=news_db user=news_user password=news_password";
    news_aggregator::storage::PostgresClient postgres_client(connection_string);
    
    if (!postgres_client.Connect()) {
        throw std::runtime_error("Failed to connect to PostgreSQL");
    }
    
    // Get news from database
    auto news_items = postgres_client.GetLatestNews(limit);
    
    // Create response with news from database
    formats::json::ValueBuilder response;
    response["status"] = "success";
    response["count"] = news_items.size();
    response["limit"] = limit;
    response["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    response["message"] = "Latest news from StorageService";
    response["source"] = "PostgreSQL Database";
    
    // Create news array from query result
    formats::json::ValueBuilder news_array;
    for (const auto& item : news_items) {
        formats::json::ValueBuilder news_item;
        news_item["id"] = item.id;
        news_item["title"] = item.title;
        news_item["content"] = item.content;
        news_item["source"] = item.source;
        news_item["category"] = item.category;
        news_item["published_at"] = item.published_at;
        if (!item.url.empty()) {
            news_item["url"] = item.url;
        }
        news_array.PushBack(news_item.ExtractValue());
    }
    response["news"] = news_array.ExtractValue();

    return formats::json::ToString(response.ExtractValue());

  } catch (const std::exception& ex) {
    LOG_ERROR() << "Error in NewsLatestHandler: " << ex.what();
    
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
