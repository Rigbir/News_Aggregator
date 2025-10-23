#include "health_handler.hpp"

#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>

namespace news_aggregator::handlers {

std::string PingHandler::HandleRequest(
    server::http::HttpRequest& request,
    server::request::RequestContext&) const {
  request.GetHttpResponse().SetContentType(http::content_type::kTextPlain);
  return "pong";
}

std::string HealthHandler::HandleRequest(
    server::http::HttpRequest& request,
    server::request::RequestContext&) const {
  
  request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);
  
  // Add CORS headers to allow browser access
  request.GetHttpResponse().SetHeader(std::string("Access-Control-Allow-Origin"), std::string("*"));
  request.GetHttpResponse().SetHeader(std::string("Access-Control-Allow-Methods"), std::string("GET, POST, OPTIONS"));
  request.GetHttpResponse().SetHeader(std::string("Access-Control-Allow-Headers"), std::string("Content-Type, Authorization"));

  try {
        // Create simple health check response
    formats::json::ValueBuilder response;
    response["status"] = "healthy";
    response["overall_health"] = "ok";
    response["healthy_services"] = 3;
    response["total_services"] = 3;
    response["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
        // Simple information about services
    response["message"] = "All services are healthy (simulated)";
    response["storage_service"] = "http://localhost:8080 - healthy";
    response["collector_service"] = "http://localhost:8081 - healthy";
    response["parser_service"] = "http://localhost:8082 - healthy";

    request.GetHttpResponse().SetStatus(server::http::HttpStatus::kOk);
    return formats::json::ToString(response.ExtractValue());

  } catch (const std::exception& ex) {
    LOG_ERROR() << "Error in HealthHandler: " << ex.what();
    
    formats::json::ValueBuilder error_response;
    error_response["status"] = "error";
    error_response["message"] = "Health check failed";
    error_response["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    request.GetHttpResponse().SetStatus(server::http::HttpStatus::kInternalServerError);
    return formats::json::ToString(error_response.ExtractValue());
  }
}

}  // namespace news_aggregator::handlers
