#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/utils/daemon_run.hpp>

#include "../handlers/health_handler.hpp"
#include "../handlers/news_latest_handler.hpp"
#include "../handlers/news_add_handler.hpp"

int main(int argc, char* argv[]) {
  const auto component_list = components::MinimalServerComponentList()
                                      .Append<news_aggregator::handlers::PingHandler>()
                                      .Append<news_aggregator::handlers::NewsLatestHandler>()
                                      .Append<news_aggregator::handlers::NewsAddHandler>();
  return utils::DaemonMain(argc, argv, component_list);
}
