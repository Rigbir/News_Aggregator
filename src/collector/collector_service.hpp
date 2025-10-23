#pragma once

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/yaml_config/schema.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <atomic>
#include <memory>
#include "http_client.hpp"
#include "rss_parser.hpp"

namespace news_aggregator::collector {

struct NewsSource {
    std::string name;
    std::string url;
    std::string category;
};

class CollectorService final : public components::LoggableComponentBase {
public:
    static constexpr std::string_view kName = "collector-service";

    CollectorService(const components::ComponentConfig& config,
                     const components::ComponentContext& component_context);

    ~CollectorService() override = default;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    void OnAllComponentsLoaded() override;
    void OnAllComponentsAreStopping() override;
    
    void StartCollectionLoop();
    void CollectNewsFromSource(const NewsSource& source);
    void SendToStorageService(const NewsSource& source, const RssItem& item);
    
    std::vector<NewsSource> news_sources_;
    std::chrono::seconds collection_interval_;
    engine::TaskWithResult<void> collection_task_;
    std::atomic<bool> should_stop_{false};
    std::unique_ptr<HttpClient> http_client_;
    std::unique_ptr<RssParser> rss_parser_;
};

}  // namespace news_aggregator::collector
