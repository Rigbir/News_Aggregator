#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/utils/daemon_run.hpp>

#include "collector_service.hpp"
#include "../handlers/collector_status_handler.hpp"

int main(int argc, char* argv[]) {
  const auto component_list = components::MinimalServerComponentList()
                                  .Append<news_aggregator::collector::CollectorService>()
                                  .Append<news_aggregator::handlers::CollectorStatusHandler>();
  return utils::DaemonMain(argc, argv, component_list);
}
