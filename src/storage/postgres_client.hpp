#pragma once

#include <string>
#include <vector>
#include <memory>
#include <postgresql@14/libpq-fe.h>

namespace news_aggregator::storage {

struct NewsItem {
    int id;
    std::string title;
    std::string content;
    std::string source;
    std::string category;
    std::string published_at;
    std::string url;
};

class PostgresClient {
public:
    PostgresClient(const std::string& connection_string);
    ~PostgresClient();
    
    bool Connect();
    void Disconnect();
    bool IsConnected() const;
    
    std::vector<NewsItem> GetLatestNews(int limit);
    int AddNews(const std::string& title, const std::string& content, 
                const std::string& source, const std::string& category, 
                const std::string& url = "");
    std::string EscapeString(const std::string& str);

private:
    std::string connection_string_;
    PGconn* connection_;
};

}  // namespace news_aggregator::storage
