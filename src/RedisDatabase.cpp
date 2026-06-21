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
    for (const auto &kv:list_store) keyList.push_back(kv.first);
    for (const auto &kv:hash_store) keyList.push_back(kv.first);
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
bool RedisDatabase::del(const std::vector<std::string>&keys, int&count){
    std::lock_guard<std::mutex> lock(db_mutex);
    count = 0;
    for (const auto &key:keys){
        if ((kv_store.find(key) != kv_store.end()) ||
            (list_store.find(key) != list_store.end()) ||
            (hash_store.find(key) != hash_store.end())) count++;
        kv_store.erase(key);
        list_store.erase(key);
        hash_store.erase(key);
        expire_store.erase(key);
    }
    return (count > 0);
}
bool RedisDatabase::rename(const std::string&oldKey, const std::string&newKey){
    std::lock_guard<std::mutex> lock(db_mutex);
    if (oldKey == newKey) return true;
    bool oldKeyFound = ((kv_store.find(oldKey) != kv_store.end()) ||
                   (list_store.find(oldKey) != list_store.end()) ||
                   (hash_store.find(oldKey) != hash_store.end()));
    if (!oldKeyFound) return false;
    bool newKeyFound = ((kv_store.find(newKey) != kv_store.end()) ||
                   (list_store.find(newKey) != list_store.end()) ||
                   (hash_store.find(newKey) != hash_store.end()));
    if (newKeyFound) return false;
    
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

// List Commands
bool RedisDatabase::lget(const std::string&key, std::vector<std::string>&result){
    std::lock_guard<std::mutex> lock(db_mutex);
    if (list_store.find(key) == list_store.end()) return false;
    result = list_store[key].vec();
    return true;
};
bool RedisDatabase::llen(const std::string&key, size_t&length){
    std::lock_guard<std::mutex>lock(db_mutex);
    if (list_store.find(key) == list_store.end()) return false;
    length = list_store[key].size();
    return true;
};
bool RedisDatabase::lpush(const std::string&key, const std::string&value, size_t&len){
    std::lock_guard<std::mutex>lock(db_mutex);
    auto&vec = list_store[key];
    vec.push_front(value);
    len = vec.size();
    return true;
};
bool RedisDatabase::rpush(const std::string&key, const std::string&value, size_t&len){
    std::lock_guard<std::mutex>lock(db_mutex);
    auto&vec = list_store[key];
    vec.push_back(value);
    len = vec.size();
    return true;
};
bool RedisDatabase::lpop(const std::string&key, const int count, std::vector<std::string>&values){
    std::lock_guard<std::mutex>lock(db_mutex);
    if (list_store.find(key) == list_store.end()) return false;
    auto&vec = list_store[key];
    int sz = std::min(count, (int)(vec.size()));
    for (int i = 0; i < sz; i++){
        values.push_back(vec.pop_front());
    }
    return (values.size() > 0);
};
bool RedisDatabase::rpop(const std::string&key, const int count, std::vector<std::string>&values){
    std::lock_guard<std::mutex>lock(db_mutex);
    if (list_store.find(key) == list_store.end()) return false;
    auto&vec = list_store[key];
    int sz = std::min(count, (int)(vec.size()));
    for (int i = 0; i < sz; i++){
        values.push_back(vec.pop_back());
    }
    return (values.size() > 0);
};
bool RedisDatabase::lrem(const std::string&key, const int count, const std::string&value, int&removedCount){
    std::lock_guard<std::mutex>lock(db_mutex);
    if (list_store.find(key) == list_store.end()) return false;
    auto&vec = list_store[key];
    if (count == 0) removedCount = vec.removeElements(value, vec.size());
    else if (count < 0) removedCount = vec.removeElements(value, std::abs(count), 1);
    else removedCount = vec.removeElements(value, count);
    return true;
};
bool RedisDatabase::lindex(const std::string&key, const int index, std::string&value){
    std::lock_guard<std::mutex>lock(db_mutex);
    if (list_store.find(key) == list_store.end()) return false;
    auto&vec = list_store[key];
    if (vec.size() == 0 || vec.size() < std::abs(index)) return false;
    value = vec.findAtIndex((vec.size() + index)%vec.size());
    return true;
};
bool RedisDatabase::lset(const std::string&key, const int index, const std::string&value){
    std::lock_guard<std::mutex>lock(db_mutex);
    if (list_store.find(key) == list_store.end()) return false;
    auto&vec = list_store[key];
    if (vec.setAtIndex(index, value)) return true;
    else return false;
};

// Hash Commands
bool RedisDatabase::hset(const std::string&key, const std::string&field, const std::string&value){
    std::lock_guard<std::mutex>lock(db_mutex);
    hash_store[key][field] = value;
    return true;
}
bool RedisDatabase::hget(const std::string&key, const std::string&field, std::string&value){
    std::lock_guard<std::mutex>lock(db_mutex);
    if (hash_store.find(key) == hash_store.end()) return false;
    auto &mp = hash_store[key];
    if (mp.find(field) == mp.end()) return false;
    value = mp[field];
    return true;
}
bool RedisDatabase::hexists(const std::string&key, const std::string&field, int&value){
    std::lock_guard<std::mutex>lock(db_mutex);
    value = 0;
    if (hash_store.find(key) == hash_store.end()) return 0;
    if (hash_store[key].find(field) == hash_store[key].end()) return 0;
    value = 1;
    return true;
}
bool RedisDatabase::hdel(const std::string&key, const std::string&field){
    std::lock_guard<std::mutex>lock(db_mutex);
    if (hash_store.find(key) == hash_store.end()) return 0;
    auto&mp = hash_store[key];
    if (mp.find(field) == mp.end()) return 0;
    mp.erase(field);
    return true;
}
bool RedisDatabase::hgetall(const std::string&key, std::vector<std::pair<std::string, std::string>>&values){
    std::lock_guard<std::mutex>lock(db_mutex);
    if (hash_store.find(key) == hash_store.end()) return 0;
    auto &mp = hash_store[key];
    for (auto&it:mp){
        values.push_back(it);
    }
    return true;
}
bool RedisDatabase::hkeys(const std::string&key, std::vector<std::string>&values){
    std::lock_guard<std::mutex>lock(db_mutex);
    if (hash_store.find(key) == hash_store.end()) return 0;
    auto&mp = hash_store[key];
    for (auto&it:mp) values.push_back(it.first);
    return true;
}
bool RedisDatabase::hvals(const std::string&key, std::vector<std::string>&values){
    std::lock_guard<std::mutex>lock(db_mutex);
    if (hash_store.find(key) == hash_store.end()) return 0;
    auto &mp = hash_store[key];
    for (auto &it:mp) values.push_back(it.second);
    return true;
}
bool RedisDatabase::hlen(const std::string&key, int&value){
    std::lock_guard<std::mutex>lock(db_mutex);
    if (hash_store.find(key) == hash_store.end()) return 0;
    value = hash_store[key].size();
    return true;
}
bool RedisDatabase::hmset(const std::string&key, const std::vector<std::string>&tokens){
    std::lock_guard<std::mutex>lock(db_mutex);
    int sz = tokens.size();
    auto &mp = hash_store[key];
    if ((sz < 4) || (sz&1)) return 0;
    for (int i = 2; i < sz; i+=2){
        mp[tokens[i]] = tokens[i+1];
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
        std::vector<std::string> vec = list.second.vec();
        for (const auto &item:vec) file << ' ' << item;
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

        if (type == 'K'){
            std::string key, val;
            getline(ss, key, ':');   // Extract key until ':'
            getline(ss, val);        // Everything after ':' is the value
            if (key.empty() || val.empty()) continue;
            key = key.substr(1);     // Remove leading space after 'K'
            kv_store[key] = val;
        }else if (type == 'L'){
            std::string key;
            ss >> key;              
            std::string item;
            if (key.empty()) continue;
            LList* items = &list_store[key];
            while (ss >> item) items->push_back(item);
        }else if (type == 'H'){
            std::string key;
            ss >> key;            
            std::unordered_map<std::string, std::string> hash;
            std::string field_val;
            while (ss >> field_val){
                size_t colonPos = field_val.find(':');
                if (colonPos != std::string::npos) {
                    hash[field_val.substr(0, colonPos)] = field_val.substr(colonPos + 1);
                }
            }
            if (!key.empty() && !hash.empty()) hash_store[key] = hash;
        }
    }
    return true;
}
