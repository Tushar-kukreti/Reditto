#ifndef REDITDATABASE_H
#define REDITDATABASE_H

#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <chrono>

class RedisDatabase{
private:
    RedisDatabase() = default;
    ~RedisDatabase() = default;
    RedisDatabase(RedisDatabase const&) = delete;
    RedisDatabase& operator=(RedisDatabase const&) = delete;
public:
    static RedisDatabase& getInstance();
    // System commands
    bool flushALL();

    // Key commands
    bool set(const std::string& key, const std::string& value);
    bool get(const std::string& key, std::string& value);
    bool type(const std::string&key, std::string& value);
    bool keys(std::vector<std::string>&keyList);
    bool expire(const std::string&key, const std::string&seconds);
    bool del(const std::vector<std::string>&keys);
    bool rename(const std::string&oldKey, const std::string&newKey);

    // persitence commands
    bool dump(const std::string&fileName);
    bool load(const std::string&fileName);

    // db
    std::mutex db_mutex;
    std::unordered_map<std::string, std::string>kv_store;
    std::unordered_map<std::string, std::vector<std::string>> list_store;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> hash_store;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> expire_store;
};
#endif