#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/utils/daemon_run.hpp>

#include "../handlers/news_handler.hpp"
#include "../handlers/health_handler.hpp"

int main(int argc, char* argv[]) {
  const auto component_list = components::MinimalServerComponentList()
                                  .Append<news_aggregator::handlers::NewsHandler>()
                                  .Append<news_aggregator::handlers::HealthHandler>();
  return utils::DaemonMain(argc, argv, component_list);
}
