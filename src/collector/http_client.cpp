#include "http_client.hpp"
#include <userver/logging/log.hpp>
#include <curl/curl.h>
#include <stdexcept>
#include <sstream>
#include <fstream>

namespace news_aggregator::collector {

// Callback function for writing data from curl
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* data) {
    size_t total_size = size * nmemb;
    data->append(static_cast<char*>(contents), total_size);
    return total_size;
}

// Callback function for writing headers
static size_t HeaderCallback(void* contents, size_t size, size_t nmemb, std::map<std::string, std::string>* headers) {
    size_t total_size = size * nmemb;
    std::string header_line(static_cast<char*>(contents), total_size);
    
    // Remove \r\n at the end
    if (header_line.length() >= 2 && header_line.substr(header_line.length() - 2) == "\r\n") {
        header_line = header_line.substr(0, header_line.length() - 2);
    }
    
    // Parse header
    size_t colon_pos = header_line.find(':');
    if (colon_pos != std::string::npos) {
        std::string key = header_line.substr(0, colon_pos);
        std::string value = header_line.substr(colon_pos + 1);
        
        // Remove spaces
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        (*headers)[key] = value;
    }
    
    return total_size;
}

HttpClient::HttpClient() : user_agent_("NewsAggregator/1.0"), default_timeout_(30) {
    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

HttpClient::~HttpClient() {
    curl_global_cleanup();
}

HttpResponse HttpClient::Get(const std::string& url, int timeout_seconds) {
    // Check if URL is file-based
    if (url.substr(0, 7) == "file://") {
        return GetFromFile(url);
    }
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
                                       const std::string& data, const std::string& content_type,
                                       int timeout_seconds) {
    HttpResponse response;
    response.success = false;
    response.status_code = 0;
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR() << "Failed to initialize curl";
        return response;
    }
    
    try {
        // Configure curl
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent_.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_seconds);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        
        // Callback for writing data
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
        
        // Callback for writing headers
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response.headers);
        
        // Configure method
        if (method == "POST") {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.length());
            
            if (!content_type.empty()) {
                struct curl_slist* headers = nullptr;
                std::string header = "Content-Type: " + content_type;
                headers = curl_slist_append(headers, header.c_str());
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            }
        }
        
        LOG_INFO() << "Making " << method << " request to: " << url;
        
        // Execute request
        CURLcode res = curl_easy_perform(curl);
        
        if (res == CURLE_OK) {
            // Get status code
            long status_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
            response.status_code = static_cast<int>(status_code);
            response.success = (status_code >= 200 && status_code < 300);
            
            LOG_INFO() << "Request completed with status: " << status_code 
                       << ", body size: " << response.body.length() << " bytes";
        } else {
            LOG_ERROR() << "Curl request failed: " << curl_easy_strerror(res);
            response.success = false;
        }
        
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Exception during HTTP request: " << ex.what();
        response.success = false;
    }
    
    curl_easy_cleanup(curl);
    return response;
}

HttpResponse HttpClient::GetFromFile(const std::string& file_url) {
    HttpResponse response;
    response.success = false;
    response.status_code = 0;
    
    try {
        // Extract file path from file:// URL
        std::string file_path = file_url.substr(7); // Remove "file://"
        
        LOG_INFO() << "Reading file: " << file_path;
        
        std::ifstream file(file_path);
        if (!file.is_open()) {
            LOG_ERROR() << "Failed to open file: " << file_path;
            response.status_code = 404;
            return response;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        
        response.body = buffer.str();
        response.status_code = 200;
        response.success = true;
        
        LOG_INFO() << "Successfully read file: " << file_path 
                   << ", size: " << response.body.length() << " bytes";
        
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error reading file: " << ex.what();
        response.success = false;
        response.status_code = 500;
    }
    
    return response;
}

} // namespace news_aggregator::collector
