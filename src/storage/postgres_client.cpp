#include "postgres_client.hpp"
#include <userver/logging/log.hpp>
#include <stdexcept>

namespace news_aggregator::storage {

PostgresClient::PostgresClient(const std::string& connection_string)
    : connection_string_(connection_string), connection_(nullptr) {
}

PostgresClient::~PostgresClient() {
    Disconnect();
}

bool PostgresClient::Connect() {
    if (connection_) {
        return true;
    }
    
    connection_ = PQconnectdb(connection_string_.c_str());
    
    if (PQstatus(connection_) != CONNECTION_OK) {
        LOG_ERROR() << "PostgreSQL connection failed: " << PQerrorMessage(connection_);
        PQfinish(connection_);
        connection_ = nullptr;
        return false;
    }
    
    LOG_INFO() << "Successfully connected to PostgreSQL database";
    return true;
}

void PostgresClient::Disconnect() {
    if (connection_) {
        PQfinish(connection_);
        connection_ = nullptr;
    }
}

bool PostgresClient::IsConnected() const {
    return connection_ && PQstatus(connection_) == CONNECTION_OK;
}

std::vector<NewsItem> PostgresClient::GetLatestNews(int limit) {
    std::vector<NewsItem> news;
    
    if (!IsConnected()) {
        LOG_ERROR() << "Not connected to PostgreSQL";
        return news;
    }
    
    std::string query = "SELECT id, title, content, source, category, published_at, url FROM news ORDER BY created_at DESC LIMIT " + std::to_string(limit);
    
    LOG_INFO() << "Executing query: " << query;
    
    PGresult* result = PQexec(connection_, query.c_str());
    
    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        LOG_ERROR() << "PostgreSQL query failed: " << PQerrorMessage(connection_);
        PQclear(result);
        return news;
    }
    
    int rows = PQntuples(result);
    LOG_INFO() << "Query returned " << rows << " rows";
    
    for (int i = 0; i < rows; ++i) {
        NewsItem item;
        item.id = std::stoi(PQgetvalue(result, i, 0));
        item.title = PQgetvalue(result, i, 1);
        item.content = PQgetvalue(result, i, 2);
        item.source = PQgetvalue(result, i, 3);
        item.category = PQgetvalue(result, i, 4);
        item.published_at = PQgetvalue(result, i, 5);
        item.url = PQgetisnull(result, i, 6) ? "" : PQgetvalue(result, i, 6);
        news.push_back(item);
        LOG_INFO() << "Retrieved news item: " << item.title << " (ID: " << item.id << ")";
    }
    
    PQclear(result);
    return news;
}

int PostgresClient::AddNews(const std::string& title, const std::string& content, 
                           const std::string& source, const std::string& category, 
                           const std::string& url) {
    if (!IsConnected()) {
        LOG_ERROR() << "Not connected to PostgreSQL";
        return -1;
    }
    
    std::string query = "INSERT INTO news (title, content, source, category, url) VALUES ($1, $2, $3, $4, $5) RETURNING id";
    
    const char* values[5] = {
        title.c_str(),
        content.c_str(),
        source.c_str(),
        category.c_str(),
        url.c_str()
    };
    
    int lengths[5] = {
        static_cast<int>(title.length()),
        static_cast<int>(content.length()),
        static_cast<int>(source.length()),
        static_cast<int>(category.length()),
        static_cast<int>(url.length())
    };
    
    int formats[5] = {0, 0, 0, 0, 0}; // text format
    
    PGresult* result = PQexecParams(connection_, query.c_str(), 5, nullptr, values, lengths, formats, 0);
    
    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        LOG_ERROR() << "PostgreSQL insert failed: " << PQerrorMessage(connection_);
        PQclear(result);
        return -1;
    }
    
    int news_id = std::stoi(PQgetvalue(result, 0, 0));
    PQclear(result);
    
    LOG_INFO() << "Successfully inserted news with ID: " << news_id;
    return news_id;
}

std::string PostgresClient::EscapeString(const std::string& str) {
    if (!IsConnected()) {
        return str;
    }
    
    char* escaped = PQescapeLiteral(connection_, str.c_str(), str.length());
    if (!escaped) {
        LOG_ERROR() << "Failed to escape string: " << PQerrorMessage(connection_);
        return str;
    }
    
    std::string result(escaped);
    PQfreemem(escaped);
    return result;
}

}  // namespace news_aggregator::storage