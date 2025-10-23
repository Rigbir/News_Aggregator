#include "rss_parser.hpp"
#include <userver/logging/log.hpp>
#include <regex>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>

namespace news_aggregator::collector {

RssParser::RssParser() {
    LOG_INFO() << "RssParser initialized";
}

RssParser::~RssParser() = default;

RssFeed RssParser::ParseRssContent(const std::string& rss_content, const std::string& source_url) {
    RssFeed feed;
    feed.link = source_url;
    
    LOG_INFO() << "Parsing RSS content, size: " << rss_content.length() << " bytes";
    
    try {
        // Extract channel information
        feed.title = ExtractText(rss_content, "title");
        feed.description = ExtractText(rss_content, "description");
        feed.language = ExtractText(rss_content, "language");
        
        // If title not found in channel, search in root
        if (feed.title.empty()) {
            feed.title = ExtractText(rss_content, "title");
        }
        
        LOG_INFO() << "Feed title: " << feed.title;
        LOG_INFO() << "Feed description: " << feed.description;
        
        // Extract all items
        std::vector<std::string> item_contents = ExtractAllText(rss_content, "item");
        
        LOG_INFO() << "Found " << item_contents.size() << " RSS items";
        
        for (const auto& item_content : item_contents) {
            RssItem item;
            
            item.title = CleanHtml(ExtractText(item_content, "title"));
            item.description = CleanHtml(ExtractText(item_content, "description"));
            item.link = ExtractText(item_content, "link");
            item.pub_date = NormalizeDate(ExtractText(item_content, "pubDate"));
            item.guid = ExtractText(item_content, "guid");
            item.category = ExtractText(item_content, "category");
            
            // If no guid, use link
            if (item.guid.empty() && !item.link.empty()) {
                item.guid = item.link;
            }
            
            // Skip empty items
            if (!item.title.empty() && !item.description.empty()) {
                feed.items.push_back(item);
                LOG_DEBUG() << "Parsed item: " << item.title;
            }
        }
        
        LOG_INFO() << "Successfully parsed " << feed.items.size() << " valid RSS items";
        
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error parsing RSS content: " << ex.what();
    }
    
    return feed;
}

RssFeed RssParser::ParseRssFile(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        LOG_ERROR() << "Failed to open RSS file: " << file_path;
        return RssFeed{};
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return ParseRssContent(buffer.str(), file_path);
}

bool RssParser::IsValidRss(const std::string& rss_content) {
    // Simple check for basic RSS elements
    return rss_content.find("<rss") != std::string::npos ||
           rss_content.find("<feed") != std::string::npos ||
           rss_content.find("<channel") != std::string::npos;
}

std::string RssParser::ExtractText(const std::string& content, const std::string& tag) {
    // First try to find CDATA section
    std::string cdata_pattern = "<" + tag + "[^>]*><!\\[CDATA\\[([\\s\\S]*?)\\]\\]></" + tag + ">";
    std::regex cdata_regex(cdata_pattern, std::regex_constants::icase);
    std::smatch cdata_match;
    
    if (std::regex_search(content, cdata_match, cdata_regex) && cdata_match.size() > 1) {
        return cdata_match.str(1);
    }
    
    // If CDATA not found, search for regular tag
    std::string pattern = "<" + tag + "[^>]*>([\\s\\S]*?)</" + tag + ">";
    std::regex regex_pattern(pattern, std::regex_constants::icase);
    std::smatch match;
    
    if (std::regex_search(content, match, regex_pattern) && match.size() > 1) {
        return match.str(1);
    }
    
    return "";
}

std::vector<std::string> RssParser::ExtractAllText(const std::string& content, const std::string& tag) {
    std::vector<std::string> results;
    
    // First search for CDATA sections
    std::string cdata_pattern = "<" + tag + "[^>]*><!\\[CDATA\\[([\\s\\S]*?)\\]\\]></" + tag + ">";
    std::regex cdata_regex(cdata_pattern, std::regex_constants::icase);
    std::sregex_iterator cdata_iter(content.begin(), content.end(), cdata_regex);
    std::sregex_iterator cdata_end;
    
    for (; cdata_iter != cdata_end; ++cdata_iter) {
        if (cdata_iter->size() > 1) {
            results.push_back(cdata_iter->str(1));
        }
    }
    
    // Then search for regular tags
    std::string pattern = "<" + tag + "[^>]*>([\\s\\S]*?)</" + tag + ">";
    std::regex regex_pattern(pattern, std::regex_constants::icase);
    std::sregex_iterator iter(content.begin(), content.end(), regex_pattern);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        if (iter->size() > 1) {
            results.push_back(iter->str(1));
        }
    }
    
    return results;
}

std::string RssParser::CleanHtml(const std::string& html) {
    if (html.empty()) {
        return "";
    }
    
    std::string cleaned = html;
    
    // Remove CDATA sections (if any)
    std::regex cdata_regex("<!\\[CDATA\\[([\\s\\S]*?)\\]\\]>");
    std::smatch cdata_match;
    if (std::regex_search(cleaned, cdata_match, cdata_regex) && cdata_match.size() > 1) {
        cleaned = cdata_match.str(1);
    }
    
    // Remove HTML tags
    std::regex tag_regex("<[^>]*>");
    cleaned = std::regex_replace(cleaned, tag_regex, "");
    
    // Decode HTML entities
    cleaned = DecodeHtmlEntities(cleaned);
    
    // Remove extra whitespace and line breaks
    std::regex whitespace_regex("\\s+");
    cleaned = std::regex_replace(cleaned, whitespace_regex, " ");
    
    // Remove leading and trailing spaces
    if (!cleaned.empty()) {
        cleaned.erase(0, cleaned.find_first_not_of(" \t\n\r"));
        if (!cleaned.empty()) {
            cleaned.erase(cleaned.find_last_not_of(" \t\n\r") + 1);
        }
    }
    
    return cleaned;
}

std::string RssParser::DecodeHtmlEntities(const std::string& text) {
    std::string decoded = text;
    
    // Common HTML entities
    std::map<std::string, std::string> entities = {
        {"&amp;", "&"},
        {"&lt;", "<"},
        {"&gt;", ">"},
        {"&quot;", "\""},
        {"&#39;", "'"},
        {"&apos;", "'"},
        {"&nbsp;", " "},
        {"&mdash;", "—"},
        {"&ndash;", "–"},
        {"&hellip;", "…"},
        {"&copy;", "©"},
        {"&reg;", "®"},
        {"&trade;", "™"},
        {"&euro;", "€"},
        {"&pound;", "£"},
        {"&yen;", "¥"},
        {"&cent;", "¢"}
    };
    
    for (const auto& entity : entities) {
        std::regex entity_regex(entity.first);
        decoded = std::regex_replace(decoded, entity_regex, entity.second);
    }
    
    return decoded;
}

std::string RssParser::NormalizeDate(const std::string& date_str) {
    if (date_str.empty()) {
        return "";
    }
    
    // Simple normalization - remove extra spaces
    std::string normalized = date_str;
    normalized.erase(0, normalized.find_first_not_of(" \t\n\r"));
    normalized.erase(normalized.find_last_not_of(" \t\n\r") + 1);
    
    return normalized;
}

} // namespace news_aggregator::collector
