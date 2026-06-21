#include "../inc/RedisCommandHandler.hpp"
#include "../inc/RedisDatabase.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <functional>
#include <unordered_map>

RedisCommandHandler::RedisCommandHandler() {}
RedisCommandHandler::~RedisCommandHandler() {}

// System level commands
static bool pingCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  response << "+PONG\r\n";
  return true;
}
static bool echoCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 2) {
    response << "-Error ECHO requires an argument\r\n";
    return false;
  }
  
  response << '$';
  int len = 0;
  for (size_t i = 1; i < tokens.size(); i++){
    len += tokens[i].size() + (int)(i != tokens.size()-1);
  }
  response << std::to_string(len) << "\r\n";
  for (size_t i = 1; i < tokens.size(); i++){
    response << tokens[i] << ((i != tokens.size() - 1) ? " " : "");
  }
  response << "\r\n";
  return true;
}
static bool flushAllCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  response << ((db.flushALL()) ? "+OK\r\n" : "-Error Failed to flush database\r\n");
  return true;
}

// Key level commands
static bool setCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 3)
    response << "-Error SET requires key and value\r\n";
  else
    response << ((db.set(tokens[1], tokens[2])) ? "+OK\r\n" : "-Error Failed to set key\r\n");
  return true;
}
static bool getCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 2)
    response << "-Error GET requires key\r\n";
  else {
    std::string value;
    if (db.get(tokens[1], value))
      response << '$' << value.size() << "\r\n" << value << "\r\n";
    else
      response << "-Error key not found\r\n";
  }
  return true;
}
static bool keysCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  std::vector<std::string>keyList;
  if (db.keys(keyList)){
    response << '*' << keyList.size() << "\r\n";
    for (const auto &key:keyList){
      response << '$' << key.size() << "\r\n" << key << "\r\n";
    }
  }else response << "-Error Failed to retrieve keys\r\n";
  return true;
}
static bool typeCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 2) response << "-Error TYPE requires key\r\n";
  else {
    std::string value;
    if (db.type(tokens[1], value)) response << '+' << value << "\r\n";
    else response << "-Error key not found\r\n";
  }
  return true;
}
static bool delCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 2)
    response << "-Error DEL requires key\r\n";
  else {
    int count = 0;
    std::vector<std::string>keys;
    for (size_t i = 1; i < tokens.size(); i++) keys.push_back(tokens[i]);
    if (db.del(keys, count)) response << ":" << count << "\r\n";
    else response << "-Error Failed to delete key\r\n";
  }
  return true;
}
static bool expireCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 3) response << "-Error EXPIRE requires key and seconds\r\n";
  else {
    int seconds = 0;
    try {
      seconds = std::stoi(tokens[2]);
      if (seconds <= 0) response << "-Error expire time is not permitable.\r\n";
      else if (db.expire(tokens[1], seconds)) response << "+OK\r\n";
      else response << "-Error Failed to set expire time\r\n";
    } catch (...){
      response << "-Error Invalid parameter for EXPIRE command: " << tokens[2] << "\r\n";
      return false;
    }
  }
  return true;
}
static bool renameCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 3) {
    response << "-Error RENAME requires old key and new key \r\n";
    return false;
  }

  if (db.rename(tokens[1], tokens[2])) response << "+OK\r\n";
  else response << "-Error Failed to rename key\r\n";
  return true;
}

// List level commands
static bool lgetCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
    if (tokens.size() < 2) {
      response << "-Error lget requires key\r\n";
      return false;
    }
    std::vector<std::string>result;
    db.lget(tokens[1], result);
    response << "*" << result.size() << "\r\n";
    for (const auto&item:result){
      response << "$" << item.size() << "\r\n" << item << "\r\n";
    }
    return true;
};
static bool llenCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
    if (tokens.size() < 2) {
      response << "-Error llen requires key\r\n";
      return false;
    }

    size_t len = 0;
    db.llen(tokens[1], len);
    response << "+" << std::to_string(len) << "\r\n";
    return true;
};
static bool lpushCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
    if (tokens.size() < 3) {
      response << "-Error lpush requires key and value\r\n";
      return false;
    }
    size_t len = 0;
    for (size_t i = 2; i < tokens.size(); i++)
      db.lpush(tokens[1], tokens[i], len);
    response << "+" << len << "\r\n";
    return true;
};
static bool rpushCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
    if (tokens.size() < 3) {
      response << "-Error rpush requires key and value\r\n";
      return false;
    }
    size_t len = 0;
    for (size_t i = 2; i < tokens.size(); i++)
      db.rpush(tokens[1], tokens[i], len);
    response << "+" << len << "\r\n";
    return true;
};
static bool lpopCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
    if (tokens.size() < 3) {
      response << "-Error lpop requires key and count\r\n";
      return false;
    }
    std::vector<std::string>result;
    try{
      int count = std::stoi(tokens[2]);
      if (db.lpop(tokens[1], count, result) == 0){
        response << "+Unable to delete, Empty List\r\n";
        return false;
      }
    }catch(...){
      response << "-Error failed to pop elements, Invalid count\r\n";
      return false;
    }
    response << "*" << result.size() << "\r\n";
    for (const auto &item:result){
      response << "$" << item.size() << "\r\n" << item << "\r\n";
    }
    return true;
};
static bool rpopCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
    if (tokens.size() < 3) {
      response << "-Error rpop requires key and count\r\n";
      return false;
    }
    std::vector<std::string>result;
    try{
      int count = std::stoi(tokens[2]);
      if (db.rpop(tokens[1], count, result) == 0){
        response << "+Unable to delete, Empty List\r\n";
        return false;
      }
    }catch(...){
      response << "-Error failed to pop elements, Invalid count\r\n";
      return false;
    }
    response << "*" << result.size() << "\r\n";
    for (const auto &item:result){
      response << "$" << item.size() << "\r\n" << item << "\r\n";
    }
    return true;
};
static bool lremCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
    if (tokens.size() < 3) {
      response << "-Error lrem requires key, count\r\n";
      return false;
    }
    int elementsRemoved = 0;
    try {
      int count = std::stoi(tokens[2]);
      db.lrem(tokens[1], count, tokens[3], elementsRemoved);
      response << "+" << elementsRemoved << "\r\n";
    }catch(...){
      response << "-Error Failed to perform lrem, Invalid count\r\n";
      return false;
    }
    return true;
};
static bool lindexCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
    if (tokens.size() < 3) {
      response << "-Error lindex requires key and index\r\n";
      return false;
    }
    try{
      int index = std::stoi(tokens[2]);
      std::string value;
      if (db.lindex(tokens[1], index, value))
        response << "$" << value.size() << "\r\n" << value << "\r\n";
      else
        response << "-Error Invalid Index, out of bound\r\n";
    }catch(...){
      response << "-Error Invalid index\r\n";
      return false;
    }
    return true;
};
static bool lsetCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
    if (tokens.size() < 3) {
      response << "-Error lset requires index and new value\r\n";
      return false;
    }
    try{
      int index = std::stoi(tokens[2]);
      if (db.lset(tokens[1], index, tokens[3]))
        response << "+OK\r\n";
      else response << "-Failed to set the provided value\r\n";
    }catch(...){
      response << "-Error Invalid index\r\n";
      return false;
    }
    return true;
};

// Hash level commands
static bool hsetCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 4){
    response << "-Error hset requires key, field and value parameter.\r\n";
    return false;
  }
  db.hset(tokens[1], tokens[2], tokens[3]);
  response << ":1\r\n";
  return true;
}
static bool hgetCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 3){
    response << "-Error hget requires key and field.\r\n";
    return false;
  }
  std::string value;
  if (db.hget(tokens[1], tokens[2], value))
    response << "$" << value.size() << "\r\n" << value << "\r\n";
  else 
    response << "+Unable to find key/field\r\n";
  return true;
}
static bool hexistsCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 3){
    response << "-Error hexists requires key and field.\r\n";
    return false;
  }
  int value = 0;
  db.hexists(tokens[1], tokens[2], value);
  response << ":" << (value > 0) << "\r\n";
  return true;
}
static bool hdelCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 3){
    response << "-Error hdel requires key and field.\r\n";
    return false;
  }
  if (db.hdel(tokens[1], tokens[2]))
    response << ":1\r\n";
  else response << "$26\r\nNo such key or field found\r\n";
  return true;
}
static bool hgetallCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 2){
    response << "-Error hset requires key.\r\n";
    return false;
  }
  std::vector<std::pair<std::string, std::string>>values;
  db.hgetall(tokens[1], values);
  response << '*' << values.size() << "\r\n";
  for (auto &val:values){
    response << '*' << 2 << "\r\n";
    response << '$' << val.first.size() << "\r\n" << val.first << "\r\n";
    response << '$' << val.second.size() << "\r\n" << val.second << "\r\n";
  }
  return true;
}
static bool hkeysCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 2){
    response << "-Error hkeys requires key.\r\n";
    return false;
  }
  std::vector<std::string>keys;
  db.hkeys(tokens[1], keys);
  response << "*" << keys.size() << "\r\n";
  for (auto &key:keys){
    response << "$" << key.size() << "\r\n" << key << "\r\n";
  }
  return true;
}
static bool hvalsCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 2){
    response << "-Error hvals requires key.\r\n";
    return false;
  }
  std::vector<std::string>values;
  db.hvals(tokens[1], values);
  response << "*" << values.size() << "\r\n";
  for (auto &val:values){
    response << "$" << val.size() << "\r\n" << val << "\r\n";
  }
  return true;
}
static bool hlenCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 2){
    response << "-Error hlen requires key.\r\n";
    return false;
  }
  int value = 0;
  db.hlen(tokens[1], value);
  response << ":" << value << "\r\n";
  return true;
}
static bool hmsetCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 4){
    response << "-Error hset requires key, and proper fields along with values to set.\r\n";
    return false;
  }
  if (tokens.size()&1){
    response << "-Error missing fields or values\r\n";
    return false;
  }
  if (db.hmset(tokens[1], tokens)) response << ":1\r\n";
  else response << ":0\r\n";
  return true;
}

using CommandFunction = std::function<bool(const std::vector<std::string>&, std::ostringstream&, RedisDatabase&)>;
static const std::unordered_map<std::string, CommandFunction> commandDispatcher{
  // system commands
  {"PING", pingCommand},
  {"ECHO", echoCommand},
  {"FLUSHALL", flushAllCommand},

  // key commands
  {"SET", setCommand},
  {"GET", getCommand},
  {"KEYS", keysCommand},
  {"TYPE", typeCommand},
  {"DEL", delCommand},
  {"EXPIRE", expireCommand},
  {"RENAME", renameCommand},

  // list commands
  {"LGET", lgetCommand},
  {"LLEN", llenCommand},
  {"LPUSH", lpushCommand},
  {"RPUSH", rpushCommand},
  {"LPOP", lpopCommand},
  {"RPOP", rpopCommand},
  {"LREM", lremCommand},
  {"LINDEX", lindexCommand},
  {"LSET", lsetCommand},

  // hash commonds
  {"HSET", hsetCommand},
  {"HGET", hgetCommand},
  {"HEXISTS", hexistsCommand},
  {"HDEL", hdelCommand},
  {"HGETALL", hgetallCommand},
  {"HKEYS", hkeysCommand},
  {"HVALS", hvalsCommand},
  {"HLEN", hlenCommand},
  {"HMSET", hmsetCommand}
};

static std::vector<std::string> parseCommand(const std::string& command) {
  size_t pos = 0;
  std::vector<std::string> response;
  if (command.empty()) return response;

  // parse with spacing if not in resp formate
  if (command[0] != '*') {
    std::istringstream input(command);
    std::string token;
    while (input >> token)
      response.push_back(token);
    return response;
  }

  // parse the resp array
  // if (command[pos] != '*') return response;

  pos++;
  size_t crlf = command.find("\r\n");
  if (crlf == std::string::npos) return response;
  int num_token = 0;
  try {
    num_token = std::stoi(command.substr(pos, crlf - pos));
  } catch (...) {
    return response;
  }
  pos = crlf + 2;

  for (int i = 0; i < num_token; i++) {
    pos++;
    crlf = command.find("\r\n", pos);
    if (crlf == std::string::npos) return response;
    int len = 0;
    try {
      len = std::stoi(command.substr(pos, crlf - pos));
    } catch (...) {
      return response;
    }
    pos = crlf + 2;

    std::string token = "";
    crlf = command.find("\r\n", pos);
    if (crlf == std::string::npos) return response;
    token = command.substr(pos, len);
    response.push_back(token);
    pos = crlf + 2;
  }
  return response;
}

std::string RedisCommandHandler::processCommand(const std::string& command) {
  std::vector<std::string> tokens = parseCommand(command);
  if (tokens.size() == 0) return "-Error Invalid Command\r\n";

  // token[0] is command, the rest are arguments
  std::string cmd = tokens[0];
  std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
  std::ostringstream response;

  // check to database
  RedisDatabase&db = RedisDatabase::getInstance();

  auto it = commandDispatcher.find(cmd);
  if (it != commandDispatcher.end()) {
    it->second(tokens, response, db);
  }else {
    // Default case for unknown command
    response << "-Error Unknown Command\r\n";
  }
  
  return (response.str());
}