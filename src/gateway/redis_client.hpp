#pragma once

#include <string>
#include <memory>
#include <hiredis/hiredis.h>

namespace news_aggregator::gateway {

class RedisClient {
public:
    RedisClient(const std::string& host, int port, int timeout_ms = 5000);
    ~RedisClient();

    bool Connect();
    void Disconnect();
    bool IsConnected() const;

    // String operations
    bool Set(const std::string& key, const std::string& value, int expire_seconds = 0);
    std::string Get(const std::string& key);
    bool Del(const std::string& key);
    bool Exists(const std::string& key);

    // JSON operations
    bool SetJson(const std::string& key, const std::string& json_value, int expire_seconds = 0);
    std::string GetJson(const std::string& key);

private:
    std::string host_;
    int port_;
    int timeout_ms_;
    redisContext* context_;
    bool connected_;
    
    void HandleRedisError(const std::string& operation);
};

} // namespace news_aggregator::gateway
