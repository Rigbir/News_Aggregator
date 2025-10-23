#include "redis_client.hpp"
#include <userver/logging/log.hpp>
#include <stdexcept>
#include <chrono>

namespace news_aggregator::gateway {

RedisClient::RedisClient(const std::string& host, int port, int timeout_ms)
    : host_(host), port_(port), timeout_ms_(timeout_ms), context_(nullptr), connected_(false) {
    LOG_INFO() << "RedisClient created for " << host << ":" << port << " with timeout " << timeout_ms << "ms";
}

RedisClient::~RedisClient() {
    Disconnect();
}

bool RedisClient::Connect() {
    if (connected_) {
        LOG_INFO() << "Redis client already connected.";
        return true;
    }

    struct timeval timeout = {timeout_ms_ / 1000, (timeout_ms_ % 1000) * 1000};
    context_ = redisConnectWithTimeout(host_.c_str(), port_, timeout);

    if (context_ == nullptr || context_->err) {
        if (context_) {
            LOG_ERROR() << "Failed to connect to Redis: " << context_->errstr;
            redisFree(context_);
            context_ = nullptr;
        } else {
            LOG_ERROR() << "Failed to allocate Redis context";
        }
        return false;
    }

    connected_ = true;
    LOG_INFO() << "Successfully connected to Redis at " << host_ << ":" << port_;
    return true;
}

void RedisClient::Disconnect() {
    if (context_) {
        redisFree(context_);
        context_ = nullptr;
        connected_ = false;
        LOG_INFO() << "Disconnected from Redis.";
    }
}

bool RedisClient::IsConnected() const {
    return connected_ && context_ != nullptr;
}

bool RedisClient::Set(const std::string& key, const std::string& value, int expire_seconds) {
    if (!IsConnected()) {
        LOG_ERROR() << "Not connected to Redis.";
        return false;
    }

    redisReply* reply;
    if (expire_seconds > 0) {
        reply = static_cast<redisReply*>(redisCommand(context_, "SET %s %s EX %d", key.c_str(), value.c_str(), expire_seconds));
    } else {
        reply = static_cast<redisReply*>(redisCommand(context_, "SET %s %s", key.c_str(), value.c_str()));
    }

    if (reply == nullptr) {
        HandleRedisError("SET");
        return false;
    }

    bool success = (reply->type == REDIS_REPLY_STATUS && strcmp(reply->str, "OK") == 0);
    freeReplyObject(reply);
    
    if (success) {
        LOG_DEBUG() << "Successfully set Redis key: " << key;
    } else {
        LOG_ERROR() << "Failed to set Redis key: " << key;
    }
    
    return success;
}

std::string RedisClient::Get(const std::string& key) {
    if (!IsConnected()) {
        LOG_ERROR() << "Not connected to Redis.";
        return "";
    }

    redisReply* reply = static_cast<redisReply*>(redisCommand(context_, "GET %s", key.c_str()));

    if (reply == nullptr) {
        HandleRedisError("GET");
        return "";
    }

    std::string result;
    if (reply->type == REDIS_REPLY_STRING) {
        result = std::string(reply->str, reply->len);
        LOG_DEBUG() << "Successfully retrieved Redis key: " << key;
    } else if (reply->type == REDIS_REPLY_NIL) {
        LOG_DEBUG() << "Redis key not found: " << key;
    } else {
        LOG_ERROR() << "Unexpected Redis reply type for GET: " << reply->type;
    }

    freeReplyObject(reply);
    return result;
}

bool RedisClient::Del(const std::string& key) {
    if (!IsConnected()) {
        LOG_ERROR() << "Not connected to Redis.";
        return false;
    }

    redisReply* reply = static_cast<redisReply*>(redisCommand(context_, "DEL %s", key.c_str()));

    if (reply == nullptr) {
        HandleRedisError("DEL");
        return false;
    }

    bool success = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
    freeReplyObject(reply);
    
    if (success) {
        LOG_DEBUG() << "Successfully deleted Redis key: " << key;
    } else {
        LOG_DEBUG() << "Redis key not found for deletion: " << key;
    }
    
    return success;
}

bool RedisClient::Exists(const std::string& key) {
    if (!IsConnected()) {
        LOG_ERROR() << "Not connected to Redis.";
        return false;
    }

    redisReply* reply = static_cast<redisReply*>(redisCommand(context_, "EXISTS %s", key.c_str()));

    if (reply == nullptr) {
        HandleRedisError("EXISTS");
        return false;
    }

    bool exists = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
    freeReplyObject(reply);
    
    LOG_DEBUG() << "Redis key exists check for " << key << ": " << (exists ? "true" : "false");
    return exists;
}

bool RedisClient::SetJson(const std::string& key, const std::string& json_value, int expire_seconds) {
    return Set(key, json_value, expire_seconds);
}

std::string RedisClient::GetJson(const std::string& key) {
    return Get(key);
}

void RedisClient::HandleRedisError(const std::string& operation) {
    if (context_ && context_->err) {
        LOG_ERROR() << "Redis error during " << operation << ": " << context_->errstr;
        // Don't disconnect on error, let the caller decide
    } else {
        LOG_ERROR() << "Redis connection lost during " << operation;
        connected_ = false;
    }
}

} // namespace news_aggregator::gateway
