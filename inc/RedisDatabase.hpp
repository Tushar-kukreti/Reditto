#ifndef REDITDATABASE_H
#define REDITDATABASE_H

#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <chrono>
#include "../lib/llist.lib.hpp"
class RedisDatabase{
private:
    RedisDatabase() = default;
    ~RedisDatabase() = default;
    RedisDatabase(RedisDatabase const&) = delete;
    RedisDatabase& operator=(RedisDatabase const&) = delete;

    // db
    std::mutex db_mutex;
    std::unordered_map<std::string, std::string>kv_store;
    std::unordered_map<std::string, LList> list_store;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> hash_store;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> expire_store;
public:
    static RedisDatabase& getInstance();
    // System commands
    bool flushALL();

    // Key commands
    bool set(const std::string& key, const std::string& value);
    bool get(const std::string& key, std::string& value);
    bool type(const std::string&key, std::string& value);
    bool keys(std::vector<std::string>&keyList);
    bool expire(const std::string&key, const int seconds);
    bool del(const std::vector<std::string>&keys, int&count);
    bool rename(const std::string&oldKey, const std::string&newKey);

    // list commands
    bool lget(const std::string&key, std::vector<std::string>&result);
    bool llen(const std::string&key, size_t&length);
    bool lpush(const std::string&key, const std::string&value, size_t&len);
    bool rpush(const std::string&key, const std::string&value, size_t&len);
    bool lpop(const std::string&key, const int count, std::vector<std::string>&values);
    bool rpop(const std::string&key, const int count, std::vector<std::string>&values);
    bool lrem(const std::string&key, const int count, const std::string&value, int&removedCount);
    bool lindex(const std::string&key, const int index, std::string&value);
    bool lset(const std::string&key, const int index, const std::string&value);

    // hash commands
    bool hset(const std::string&key, const std::string&field, const std::string&value);
    bool hget(const std::string&key, const std::string&field, std::string&value);
    bool hexists(const std::string&key, const std::string&field, int&value);
    bool hdel(const std::string&key, const std::string&field);
    bool hgetall(const std::string&key, std::vector<std::pair<std::string, std::string>>&values);
    bool hkeys(const std::string&key, std::vector<std::string>&values);
    bool hvals(const std::string&key, std::vector<std::string>&values);
    bool hlen(const std::string&key, int&value);
    bool hmset(const std::string&key, const std::vector<std::string>&tokens);

    // persitence commands
    bool dump(const std::string&fileName);
    bool load(const std::string&fileName);
};
#endif