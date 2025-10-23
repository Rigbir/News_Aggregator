#include "http_client.hpp"
#include <userver/logging/log.hpp>
#include <curl/curl.h>
#include <stdexcept>

namespace news_aggregator::gateway {

// Callback function for writing data from curl
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* data) {
    data->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

// Callback function for writing headers
static size_t HeaderCallback(void* contents, size_t size, size_t nmemb, std::map<std::string, std::string>* headers) {
    std::string header_line(static_cast<char*>(contents), size * nmemb);
    size_t colon_pos = header_line.find(':');
    if (colon_pos != std::string::npos) {
        std::string key = header_line.substr(0, colon_pos);
        std::string value = header_line.substr(colon_pos + 1);
        // Remove spaces and line breaks
        key.erase(0, key.find_first_not_of(" \t\r\n"));
        key.erase(key.find_last_not_of(" \t\r\n") + 1);
        value.erase(0, value.find_first_not_of(" \t\r\n"));
        value.erase(value.find_last_not_of(" \t\r\n") + 1);
        (*headers)[key] = value;
    }
    return size * nmemb;
}

HttpClient::HttpClient() : user_agent_("API-Gateway/1.0"), default_timeout_(30) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    LOG_INFO() << "HttpClient initialized for API Gateway";
}

HttpClient::~HttpClient() {
    curl_global_cleanup();
}

HttpResponse HttpClient::Get(const std::string& url, int timeout_seconds) {
    return PerformRequest(url, "GET", "", "", timeout_seconds);
}

HttpResponse HttpClient::Post(const std::string& url, const std::string& data, 
                              const std::string& content_type, int timeout_seconds) {
    return PerformRequest(url, "POST", data, content_type, timeout_seconds);
}

void HttpClient::SetUserAgent(const std::string& user_agent) {
    user_agent_ = user_agent;
}

void HttpClient::SetTimeout(int timeout_seconds) {
    default_timeout_ = timeout_seconds;
}

HttpResponse HttpClient::PerformRequest(const std::string& url, const std::string& method,
                                       const std::string& data, 
                                       const std::string& content_type,
                                       int timeout_seconds) {
    HttpResponse response;
    response.success = false;
    response.status_code = 0;
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR() << "Failed to initialize CURL";
        return response;
    }

    try {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response.headers);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent_.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_seconds);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // WARNING: For testing only, disable in production
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); // WARNING: For testing only, disable in production

        if (method == "POST") {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.length());
            
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, ("Content-Type: " + content_type).c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }

        LOG_INFO() << "Making " << method << " request to: " << url;
        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            LOG_ERROR() << "Curl request failed: " << curl_easy_strerror(res);
            return response;
        }

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        response.status_code = static_cast<int>(http_code);
        response.success = (response.status_code >= 200 && response.status_code < 300);
        
        LOG_INFO() << "Request completed with status: " << response.status_code 
                   << ", body size: " << response.body.length() << " bytes";

    } catch (const std::exception& ex) {
        LOG_ERROR() << "Exception during HTTP request: " << ex.what();
        response.success = false;
    }
    
    curl_easy_cleanup(curl);
    return response;
}

} // namespace news_aggregator::gateway
