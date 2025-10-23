#pragma once

#include <string>
#include <map>

namespace news_aggregator::gateway {

struct HttpResponse {
    bool success;
    int status_code;
    std::string body;
    std::map<std::string, std::string> headers;
};

class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    HttpResponse Get(const std::string& url, int timeout_seconds = 30);
    HttpResponse Post(const std::string& url, const std::string& data, 
                      const std::string& content_type = "application/json", 
                      int timeout_seconds = 30);

    // Utilities
    void SetUserAgent(const std::string& user_agent);
    void SetTimeout(int timeout_seconds);

private:
    std::string user_agent_;
    int default_timeout_;
    
    // Internal methods for working with libcurl
    HttpResponse PerformRequest(const std::string& url, const std::string& method,
                               const std::string& data = "", 
                               const std::string& content_type = "",
                               int timeout_seconds = 30);
};

} // namespace news_aggregator::gateway
