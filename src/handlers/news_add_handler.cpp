#include "news_add_handler.hpp"

#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/parser/parser.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>
#include "../storage/postgres_client.hpp"

namespace news_aggregator::handlers {

std::string NewsAddHandler::HandleRequest(
    server::http::HttpRequest& request,
    server::request::RequestContext&) const {
  
  request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);

  try {
    // Get data from POST request
    const auto& body = request.RequestBody();
    LOG_INFO() << "Received news data from CollectorService: " << body;
    
    // Parse JSON from request body
    auto request_body = formats::json::FromString(body);
    
    std::string title = request_body["title"].As<std::string>();
    std::string content = request_body["content"].As<std::string>();
    std::string source = request_body["source"].As<std::string>();
    std::string category = request_body["category"].As<std::string>();
    std::string url = request_body.HasMember("url") ? request_body["url"].As<std::string>() : "";
    
    LOG_INFO() << "Saving news to PostgreSQL database: " << title;
    
    // Create PostgreSQL client
    std::string connection_string = "host=localhost port=5432 dbname=news_db user=news_user password=news_password";
    news_aggregator::storage::PostgresClient postgres_client(connection_string);
    
    if (!postgres_client.Connect()) {
        throw std::runtime_error("Failed to connect to PostgreSQL");
    }
    
    // Save news to database
    int news_id = postgres_client.AddNews(title, content, source, category, url);
    
    formats::json::ValueBuilder response;
    response["status"] = "success";
    response["message"] = "News successfully saved to database";
    response["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    response["saved_count"] = 1;
    response["news_id"] = news_id;
    response["source"] = "CollectorService";

    return formats::json::ToString(response.ExtractValue());

  } catch (const std::exception& ex) {
    LOG_ERROR() << "Error in NewsAddHandler: " << ex.what();
    
    formats::json::ValueBuilder error_response;
    error_response["status"] = "error";
    error_response["message"] = "Failed to save news to database";
    error_response["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    request.GetHttpResponse().SetStatus(server::http::HttpStatus::kInternalServerError);
    return formats::json::ToString(error_response.ExtractValue());
  }
}

}  // namespace news_aggregator::handlers
