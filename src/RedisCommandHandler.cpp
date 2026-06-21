#include "../inc/RedisCommandHandler.hpp"
#include "../inc/RedisDatabase.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <functional>
#include <unordered_map>
#include "../lib/response.lib.hpp"

RedisCommandHandler::RedisCommandHandler() {}
RedisCommandHandler::~RedisCommandHandler() {}

// System level commands
static bool pingCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  response << RESPONSE::NOTIFY("PONG");
  return true;
}
static bool echoCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 2) {
    response << RESPONSE::ERROR("Error ECHO requires an argument");
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
  response << ((db.flushALL()) ? RESPONSE::NOTIFY("OK") : RESPONSE::ERROR("Error Failed to flush database"));
  return true;
}

// Key level commands
static bool setCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 3)
    response << RESPONSE::ERROR("Error SET requires key and value");
  else
    response << ((db.set(tokens[1], tokens[2])) ? RESPONSE::NOTIFY("OK") : RESPONSE::ERROR("Error Failed to set key"));
  return true;
}
static bool getCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 2)
    response << RESPONSE::ERROR("Error GET requires key");
  else {
    std::string value;
    if (db.get(tokens[1], value))
      response << RESPONSE::STRING(value);
    else
      response << RESPONSE::ERROR("Error key not found");
  }
  return true;
}
static bool keysCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  std::vector<std::string>keyList;
  if (db.keys(keyList)){
    response << RESPONSE::ARRAY(keyList);
  }else response << RESPONSE::ERROR("Error Failed to retrieve keys");
  return true;
}
static bool typeCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 2) response << RESPONSE::ERROR("Error TYPE requires key");
  else {
    std::string value;
    if (db.type(tokens[1], value)) response << RESPONSE::NOTIFY(value);
    else response << RESPONSE::ERROR("Error key not found");
  }
  return true;
}
static bool delCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 2)
    response << RESPONSE::ERROR("Error DEL requires key");
  else {
    int count = 0;
    std::vector<std::string>keys;
    for (size_t i = 1; i < tokens.size(); i++) keys.push_back(tokens[i]);
    if (db.del(keys, count)) response << RESPONSE::NUMBER(count);
    else response << RESPONSE::ERROR("Error Failed to delete key");
  }
  return true;
}
static bool expireCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 3) response << RESPONSE::ERROR("Error EXPIRE requires key and seconds");
  else {
    int seconds = 0;
    try {
      seconds = std::stoi(tokens[2]);
      if (seconds <= 0) response << RESPONSE::ERROR("Error expire time is not permitable.");
      else if (db.expire(tokens[1], seconds)) response << RESPONSE::NOTIFY("OK");
      else response << RESPONSE::ERROR("Error Failed to set expire time");
    } catch (...){
      response << "-Error Invalid parameter for EXPIRE command: " << tokens[2] << "\r\n";
      return false;
    }
  }
  return true;
}
static bool renameCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 3) {
    response << RESPONSE::ERROR("Error RENAME requires old key and new key");
    return false;
  }

  if (db.rename(tokens[1], tokens[2])) response << RESPONSE::NOTIFY("OK");
  else response << RESPONSE::ERROR("Error Failed to rename key");
  return true;
}

// List level commands
static bool lgetCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
    if (tokens.size() < 2) {
      response << RESPONSE::ERROR("Error lget requires key");
      return false;
    }
    std::vector<std::string>result;
    db.lget(tokens[1], result);
    response << RESPONSE::ARRAY(result);
    return true;
};
static bool llenCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
    if (tokens.size() < 2) {
      response << RESPONSE::ERROR("Error llen requires key");
      return false;
    }

    size_t len = 0;
    db.llen(tokens[1], len);
    response << RESPONSE::NUMBER(len);
    return true;
};
static bool lpushCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
    if (tokens.size() < 3) {
      response << RESPONSE::ERROR("Error lpush requires key and value");
      return false;
    }
    size_t len = 0;
    for (size_t i = 2; i < tokens.size(); i++)
      db.lpush(tokens[1], tokens[i], len);
    response << RESPONSE::NUMBER((int)len);
    return true;
};
static bool rpushCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
    if (tokens.size() < 3) {
      response << RESPONSE::ERROR("Error rpush requires key and value");
      return false;
    }
    size_t len = 0;
    for (size_t i = 2; i < tokens.size(); i++)
      db.rpush(tokens[1], tokens[i], len);
    response << RESPONSE::NUMBER((int)len);
    return true;
};
static bool lpopCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
    if (tokens.size() < 3) {
      response << RESPONSE::ERROR("Error lpop requires key and count");
      return false;
    }
    std::vector<std::string>result;
    try{
      int count = std::stoi(tokens[2]);
      if (db.lpop(tokens[1], count, result) == 0){
        response << RESPONSE::NOTIFY("Unable to delete, Empty List");
        return false;
      }
    }catch(...){
      response << RESPONSE::ERROR("Error failed to pop elements, Invalid count");
      return false;
    }
    response << RESPONSE::ARRAY(result);
    return true;
};
static bool rpopCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
    if (tokens.size() < 3) {
      response << RESPONSE::ERROR("Error rpop requires key and count");
      return false;
    }
    std::vector<std::string>result;
    try{
      int count = std::stoi(tokens[2]);
      if (db.rpop(tokens[1], count, result) == 0){
        response << RESPONSE::NOTIFY("Unable to delete, Empty List");
        return false;
      }
    }catch(...){
      response << RESPONSE::ERROR("Error failed to pop elements, Invalid count");
      return false;
    }
    response << RESPONSE::ARRAY(result);
    return true;
};
static bool lremCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
    if (tokens.size() < 3) {
      response << RESPONSE::ERROR("Error lrem requires key, count");
      return false;
    }
    int elementsRemoved = 0;
    try {
      int count = std::stoi(tokens[2]);
      db.lrem(tokens[1], count, tokens[3], elementsRemoved);
      response << RESPONSE::NUMBER(elementsRemoved);
    }catch(...){
      response << RESPONSE::ERROR("Error Failed to perform lrem, Invalid count");
      return false;
    }
    return true;
};
static bool lindexCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
    if (tokens.size() < 3) {
      response << RESPONSE::ERROR("Error lindex requires key and index");
      return false;
    }
    try{
      int index = std::stoi(tokens[2]);
      std::string value;
      if (db.lindex(tokens[1], index, value))
        response << RESPONSE::STRING(value);
      else
        response << RESPONSE::ERROR("Error Invalid Index, out of bound");
    }catch(...){
      response << RESPONSE::ERROR("Error Invalid index");
      return false;
    }
    return true;
};
static bool lsetCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
    if (tokens.size() < 3) {
      response << RESPONSE::ERROR("Error lset requires index and new value");
      return false;
    }
    try{
      int index = std::stoi(tokens[2]);
      if (db.lset(tokens[1], index, tokens[3]))
        response << RESPONSE::NOTIFY("OK");
      else response << RESPONSE::ERROR("Error, Failed to set the provided value");
    }catch(...){
      response << RESPONSE::ERROR("Error Invalid index");
      return false;
    }
    return true;
};

// Hash level commands
static bool hsetCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 4){
    response << RESPONSE::ERROR("Error hset requires key, field and value parameter.");
    return false;
  }
  db.hset(tokens[1], tokens[2], tokens[3]);
  response << RESPONSE::NUMBER(1);
  return true;
}
static bool hgetCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 3){
    response << RESPONSE::ERROR("Error hget requires key and field.");
    return false;
  }
  std::string value;
  if (db.hget(tokens[1], tokens[2], value))
    response << RESPONSE::STRING(value);
  else 
    response << RESPONSE::NOTIFY("Unable to find key/field");
  return true;
}
static bool hexistsCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 3){
    response << RESPONSE::ERROR("Error hexists requires key and field.");
    return false;
  }
  int value = 0;
  db.hexists(tokens[1], tokens[2], value);
  response << RESPONSE::NUMBER(value > 0);
  return true;
}
static bool hdelCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 3){
    response << RESPONSE::ERROR("Error hdel requires key and field.");
    return false;
  }
  if (db.hdel(tokens[1], tokens[2]))
    response << RESPONSE::NUMBER(1);
  else response << RESPONSE::NOTIFY("No such key or field found");
  return true;
}
static bool hgetallCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 2){
    response << RESPONSE::ERROR("Error hset requires key.");
    return false;
  }
  std::vector<std::pair<std::string, std::string>>values;
  db.hgetall(tokens[1], values);
  response << RESPONSE::ARRAYPAIR(values);
  return true;
}
static bool hkeysCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 2){
    response << RESPONSE::ERROR("Error hkeys requires key.");
    return false;
  }
  std::vector<std::string>keys;
  db.hkeys(tokens[1], keys);
  response << RESPONSE::ARRAY(keys);
  return true;
}
static bool hvalsCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 2){
    response << RESPONSE::ERROR("Error hvals requires key.");
    return false;
  }
  std::vector<std::string>values;
  db.hvals(tokens[1], values);
  response << RESPONSE::ARRAY(values);
  return true;
}
static bool hlenCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 2){
    response << RESPONSE::ERROR("Error hlen requires key.");
    return false;
  }
  int value = 0;
  db.hlen(tokens[1], value);
  response << RESPONSE::NUMBER(value);
  return true;
}
static bool hmsetCommand(const std::vector<std::string>& tokens, std::ostringstream&response, RedisDatabase&db){
  if (tokens.size() < 4){
    response << RESPONSE::ERROR("Error hset requires key, and proper fields along with values to set.");
    return false;
  }
  if (tokens.size()&1){
    response << RESPONSE::ERROR("Error missing fields or values");
    return false;
  }
  response << RESPONSE::NUMBER(db.hmset(tokens[1], tokens));
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
  if (tokens.size() == 0) return RESPONSE::ERROR("Error Invalid Command");

  // token[0] is command, the rest are arguments
  std::string cmd = tokens[0];
  std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
  std::ostringstream response;

  // check to database
  RedisDatabase&db = RedisDatabase::getInstance();

  auto it = commandDispatcher.find(cmd);
  if (it != commandDispatcher.end()) {
    db.syncExpiry();
    it->second(tokens, response, db);
  }else {
    // Default case for unknown command
    response << RESPONSE::ERROR("Error Unknown Command");
  }
  
  return (response.str());
}