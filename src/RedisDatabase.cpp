#include "../inc/RedisDatabase.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

RedisDatabase& RedisDatabase::getInstance(){
    static RedisDatabase instance;
    return instance;
}

// System commands
bool RedisDatabase::flushALL(){
    std::lock_guard<std::mutex> lock(db_mutex);
    kv_store.clear();
    list_store.clear();
    hash_store.clear();
    return true;
}

// Key commands
bool RedisDatabase::set(const std::string& key, const std::string& value){   
    std::lock_guard<std::mutex> lock(db_mutex);
    kv_store[key] = value;
    return true;
}
bool RedisDatabase::get(const std::string& key, std::string& value){
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = kv_store.find(key);
    if (it != kv_store.end()) value = it->second;
    else return false;
    return true;
}
bool RedisDatabase::type(const std::string&key, std::string& value){
    std::lock_guard<std::mutex> lock(db_mutex);
    if (kv_store.find(key) != kv_store.end()) value = "string";
    else if (list_store.find(key) != list_store.end()) value = "list";
    else if (hash_store.find(key) != hash_store.end()) value = "hash";
    else return false;
    return true;
}
bool RedisDatabase::keys(std::vector<std::string>&keyList){
    std::lock_guard<std::mutex> lock(db_mutex);
    for (const auto &kv:kv_store) keyList.push_back(kv.first);
    return true;
}
bool RedisDatabase::expire(const std::string&key, const int seconds){
    std::lock_guard<std::mutex> lock(db_mutex);
    bool exists = ((kv_store.find(key) != kv_store.end()) 
                || (list_store.find(key) != list_store.end()))
                || (hash_store.find(key) != hash_store.end());
    if (!exists) return false;
    expire_store[key] = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    return true;
}
bool RedisDatabase::del(const std::vector<std::string>&keys){
    std::lock_guard<std::mutex> lock(db_mutex);
    for (const auto &key:keys){
        kv_store.erase(key);
        list_store.erase(key);
        hash_store.erase(key);
    }
    return true;
}
bool RedisDatabase::rename(const std::string&oldKey, const std::string&newKey){
    std::lock_guard<std::mutex> lock(db_mutex);
    bool exists = ((kv_store.find(oldKey) != kv_store.end()) ||
                   (list_store.find(oldKey) != list_store.end()) ||
                   (hash_store.find(oldKey) != hash_store.end()));
    if (!exists) return false;
    
    if (kv_store.find(oldKey) != kv_store.end()){
        kv_store[newKey] = kv_store[oldKey];
        kv_store.erase(oldKey);
    }
    
    if (list_store.find(oldKey) != list_store.end()){
        list_store[newKey] = list_store[oldKey];
        list_store.erase(oldKey);
    }
    
    if (hash_store.find(oldKey) != hash_store.end()){
        hash_store[newKey] = hash_store[oldKey];
        hash_store.erase(oldKey);
    }

    if (expire_store.find(oldKey) != expire_store.end()){
        expire_store[newKey] = expire_store[oldKey];
        expire_store.erase(oldKey);
    }
    return true;
}

// Persistence Commands
bool RedisDatabase::dump(const std::string&fileName){
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ofstream file(fileName, std::ios::binary);
    
    if (!file.is_open()) return false;

    for (const auto &kv:kv_store){
        file << "K " << kv.first << ':' << kv.second << '\n';
    }

    for (const auto &list:list_store){
        file << "L " << list.first;
        for (const auto &item:list.second) file << ' ' << item;
        file << '\n';
    }

    for (const auto &hash:hash_store){
        file << "H " << hash.first;
        for (const auto &kv:hash.second) file << ' ' << kv.first << ':' << kv.second;
        file << '\n';
    }
    return true;
}
bool RedisDatabase::load(const std::string&fileName){
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ifstream file(fileName, std::ios::binary);

    if (!file.is_open()) return false;
    std::string line;
    while (std::getline(file, line)){
        if (line.empty()) continue;
        std::stringstream ss(line);
        char type; ss >> type;
        std::string key, val;

        if (type == 'K'){
            std::string key, val;
            getline(ss, key, ':');
            getline(ss, val);
            if (key.empty() || val.empty()) continue;
            key = key.substr(1); // Remove leading space
            kv_store[key] = val;
        }else if (type == 'L'){
            std::vector<std::string>items;
            while (ss >> val) items.push_back(val);
            if (key.empty() || items.empty()) continue;
            key = key.substr(1); // Remove leading space
            list_store[key] = items;
        }else if (type == 'H'){
            std::unordered_map<std::string, std::string> hash;
            while (ss >> val){
                size_t colonPos = val.find(':');
                if (colonPos != std::string::npos) {
                    hash[val.substr(0, colonPos)] = val.substr(colonPos + 1);
                }
            }
            if (!key.empty() && !hash.empty()) hash_store[key] = hash;
        }
    }
    return true;
}
