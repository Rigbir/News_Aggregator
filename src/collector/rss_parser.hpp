#pragma once

#include <string>
#include <vector>
#include <memory>

namespace news_aggregator::collector {

struct RssItem {
    std::string title;
    std::string description;
    std::string link;
    std::string pub_date;
    std::string guid;
    std::string category;
};

struct RssFeed {
    std::string title;
    std::string description;
    std::string link;
    std::string language;
    std::vector<RssItem> items;
};

class RssParser {
public:
    RssParser();
    ~RssParser();

    // Parse RSS from string
    RssFeed ParseRssContent(const std::string& rss_content, const std::string& source_url = "");
    
    // Parse RSS from file
    RssFeed ParseRssFile(const std::string& file_path);
    
    // Validate RSS content
    bool IsValidRss(const std::string& rss_content);

private:
    // Internal parsing methods
    std::string ExtractText(const std::string& content, const std::string& tag);
    std::vector<std::string> ExtractAllText(const std::string& content, const std::string& tag);
    std::string CleanHtml(const std::string& html);
    std::string DecodeHtmlEntities(const std::string& text);
    std::string NormalizeDate(const std::string& date_str);
};

} // namespace news_aggregator::collector
