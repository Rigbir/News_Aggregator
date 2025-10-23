#include "collector_service.hpp"

#include <userver/logging/log.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>

namespace news_aggregator::collector {

CollectorService::CollectorService(const components::ComponentConfig& config,
                                   const components::ComponentContext& component_context)
    : LoggableComponentBase(config, component_context),
      collection_interval_(config["collection_interval_seconds"].As<std::chrono::seconds>(60)) {
    
    // Load news sources from configuration
    const auto sources_config = config["news_sources"];
    for (const auto& source_config : sources_config) {
        NewsSource source;
        source.name = source_config["name"].As<std::string>();
        source.url = source_config["url"].As<std::string>();
        source.category = source_config["category"].As<std::string>("general");
        news_sources_.push_back(source);
    }
    
    LOG_INFO() << "CollectorService initialized with " << news_sources_.size() 
               << " news sources, collection interval: " << collection_interval_.count() << "s";
}

void CollectorService::OnAllComponentsLoaded() {
    LOG_INFO() << "CollectorService: All components loaded, initializing HTTP client and RSS parser";
    
    // Initialize HTTP client and RSS parser
    http_client_ = std::make_unique<HttpClient>();
    rss_parser_ = std::make_unique<RssParser>();
    
    // Configure HTTP client
    http_client_->SetUserAgent("NewsAggregator/1.0 (RSS Reader)");
    http_client_->SetTimeout(30);
    
    LOG_INFO() << "CollectorService: Starting news collection loop";
    StartCollectionLoop();
}

void CollectorService::OnAllComponentsAreStopping() {
    LOG_INFO() << "CollectorService: Stopping news collection";
    should_stop_ = true;
    if (collection_task_.IsValid()) {
        collection_task_.Wait();
    }
}

void CollectorService::StartCollectionLoop() {
    collection_task_ = engine::AsyncNoSpan([this]() {
        LOG_INFO() << "Starting periodic news collection loop";
        
        while (!should_stop_) {
            try {
                LOG_INFO() << "Starting news collection cycle from " << news_sources_.size() << " sources";
                
                for (const auto& source : news_sources_) {
                    if (should_stop_) break;
                    CollectNewsFromSource(source);
                }
                
                LOG_INFO() << "News collection cycle completed, sleeping for " 
                           << collection_interval_.count() << " seconds";
                
                // Wait until next cycle or until stop
                for (int i = 0; i < collection_interval_.count() && !should_stop_; ++i) {
                    engine::SleepFor(std::chrono::seconds(1));
                }
                
            } catch (const std::exception& ex) {
                LOG_ERROR() << "Error in news collection loop: " << ex.what();
                engine::SleepFor(std::chrono::seconds(5)); // Wait 5 seconds before retry
            }
        }
        
        LOG_INFO() << "News collection loop stopped";
    });
}

void CollectorService::CollectNewsFromSource(const NewsSource& source) {
    LOG_INFO() << "Collecting from source: " << source.name << " (" << source.url << ")";
    
    try {
        // Make real HTTP request to RSS
        LOG_INFO() << "Making HTTP request to RSS feed: " << source.url;
        auto response = http_client_->Get(source.url, 30);
        
        if (!response.success) {
            LOG_ERROR() << "Failed to fetch RSS from " << source.name 
                       << ", status: " << response.status_code;
            return;
        }
        
        LOG_INFO() << "Successfully fetched RSS from " << source.name 
                   << ", content size: " << response.body.length() << " bytes";
        
        // Parse RSS data
        LOG_INFO() << "Parsing RSS content for source: " << source.name;
        auto rss_feed = rss_parser_->ParseRssContent(response.body, source.url);
        
        if (rss_feed.items.empty()) {
            LOG_WARNING() << "No RSS items found in feed from " << source.name;
            return;
        }
        
        LOG_INFO() << "Successfully parsed " << rss_feed.items.size() 
                   << " RSS items from " << source.name;
        
        // Send each news item to StorageService
        for (const auto& item : rss_feed.items) {
            LOG_INFO() << "Sending news item to StorageService: " << item.title;
            SendToStorageService(source, item);
        }
        
        // Also send to Kafka for ParserService (if needed)
        LOG_INFO() << "Sending RSS content to Kafka topic 'raw_news' from source: " << source.name;
        // TODO: Implement Kafka sending
        
        LOG_INFO() << "Successfully collected and sent " << rss_feed.items.size() 
                   << " news items from " << source.name;
        
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error collecting news from " << source.name << ": " << ex.what();
    }
}

void CollectorService::SendToStorageService(const NewsSource& source, const RssItem& item) {
    LOG_INFO() << "Sending news item from " << source.name << " to StorageService: " << item.title;
    
    try {
        // For now simulate HTTP POST request to StorageService
        // In real implementation there would be:
        // auto http_client = context.FindComponent<clients::http::Client>("http-client");
        // auto response = http_client->CreateRequest()
        //     .post("http://localhost:8080/news/add")
        //     .data(json_data)
        //     .timeout(std::chrono::milliseconds{5000})
        //     .perform();
        
        // Create JSON data for sending
        formats::json::ValueBuilder builder;
        builder["title"] = item.title;
        builder["content"] = item.description;
        builder["source"] = source.name;
        builder["category"] = source.category;
        builder["url"] = item.link;
        builder["published_at"] = item.pub_date;
        
        std::string news_data = formats::json::ToString(builder.ExtractValue());
        
        LOG_INFO() << "Making real POST request to StorageService with data: " << news_data;
        
        // Make real HTTP POST request to StorageService
        auto response = http_client_->Post("http://localhost:8080/news/add", news_data, 
                                          "application/json", 10);
        
        if (response.success) {
            LOG_INFO() << "Successfully sent news item to StorageService: " << item.title 
                       << ", status: " << response.status_code;
        } else {
            LOG_ERROR() << "Failed to send news item to StorageService: " << item.title 
                        << ", status: " << response.status_code 
                        << ", body: " << response.body;
        }
        
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error sending news item to StorageService: " << ex.what();
    }
}

yaml_config::Schema CollectorService::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: CollectorService component config
additionalProperties: false
properties:
    collection_interval_seconds:
        type: integer
        description: interval in seconds between news collection cycles
        defaultDescription: 60
    news_sources:
        type: array
        description: list of news sources to collect from
        items:
            type: object
            description: news source configuration
            additionalProperties: false
            properties:
                name:
                    type: string
                    description: source name
                url:
                    type: string
                    description: RSS feed URL
                category:
                    type: string
                    description: news category
                    defaultDescription: general
)");
}

}  // namespace news_aggregator::collector
