#include "../inc/RedisDatabase.hpp"
#include <fstream>
#include <sstream>

RedisDatabase& RedisDatabase::getInstance(){
    static RedisDatabase instance;
    return instance;
}

// System commands
bool RedisDatabase::flushALL(){return true;}

// Key commands
bool RedisDatabase::set(const std::string& key, const std::string& value){return true;}
bool RedisDatabase::get(const std::string& key, std::string& value){return true;}
bool RedisDatabase::type(const std::string&key, std::string& value){return true;}
bool RedisDatabase::keys(std::vector<std::string>&keyList){return true;}
bool RedisDatabase::expire(const std::string&key, const std::string&seconds){return true;}
bool RedisDatabase::del(const std::vector<std::string>&keys){return true;}
bool RedisDatabase::rename(const std::string&oldKey, const std::string&newKey){return true;}

// Persistence Commands
bool RedisDatabase::dump(const std::string&fileName){
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ofstream file(fileName, std::ios::binary);
    
    if (!file.is_open()) return false;

    for (const auto &kv:kv_store){
        file << 'K' << kv.first << ':' << kv.second << '\n';
    }

    for (const auto &list:list_store){
        file << 'L' << list.first;
        for (const auto &item:list.second) file << ' ' << item;
        file << '\n';
    }

    for (const auto &hash:hash_store){
        file << 'H' << hash.first;
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
            getline(ss, key, ':');
            getline(ss, val);
            if (key.empty() || val.empty()) continue;
            kv_store[key] = val;
        }else if (type == 'L'){
            std::string key, item;
            std::vector<std::string>items;
            ss >> key;
            while (ss >> item) items.push_back(item);
            if (key.empty() || items.empty()) continue;
            list_store[key] = items;
        }else if (type == 'H'){
            std::string key, pair;
            std::unordered_map<std::string, std::string> hash;
            ss >> key;
            while (ss >> pair){
                size_t colonPos = pair.find(':');
                if (colonPos != std::string::npos) {
                    hash[pair.substr(0, colonPos)] = pair.substr(colonPos + 1);
                }
            }
            if (!key.empty() && !hash.empty()) hash_store[key] = hash;
        }
    }
    return true;
}
