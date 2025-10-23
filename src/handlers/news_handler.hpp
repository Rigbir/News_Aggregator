#pragma once

#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utest/using_namespace_userver.hpp>

namespace news_aggregator::handlers {

class NewsHandler final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "news-handler";

  using HttpHandlerBase::HttpHandlerBase;

  std::string HandleRequest(
      server::http::HttpRequest& request,
      server::request::RequestContext&) const override;
};

}  // namespace news_aggregator::handlers