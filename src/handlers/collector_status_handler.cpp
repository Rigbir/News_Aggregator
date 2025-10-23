#include "collector_status_handler.hpp"

#include <userver/formats/json/value_builder.hpp>

namespace news_aggregator::handlers {

std::string CollectorStatusHandler::HandleRequest(
    server::http::HttpRequest& request,
    server::request::RequestContext&) const {
  request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);

  formats::json::ValueBuilder builder;
  builder["status"] = "running";
  builder["service"] = "CollectorService";
  builder["message"] = "News collection service is operational";
  builder["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();

  return formats::json::ToString(builder.ExtractValue());
}

}  // namespace news_aggregator::handlers
