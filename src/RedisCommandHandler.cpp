#include "../inc/RedisCommandHandler.hpp"
#include "../inc/RedisDatabase.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <functional>

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
    std::vector<std::string>keys;
    for (size_t i = 1; i < tokens.size(); i++) keys.push_back(tokens[i]);
    if (db.del(keys)) response << "+OK\r\n";
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
      if (db.expire(tokens[1], seconds)) response << "+OK\r\n";
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

using CommandFunction = std::function<bool(const std::vector<std::string>&, std::ostringstream&, RedisDatabase&)>;
static const std::unordered_map<std::string, CommandFunction> commandDispatcher{
  {"PING", pingCommand},
  {"ECHO", echoCommand},
  {"FLUSHALL", flushAllCommand},
  {"SET", setCommand},
  {"GET", getCommand},
  {"KEYS", keysCommand},
  {"TYPE", typeCommand},
  {"DEL", delCommand},
  {"EXPIRE", expireCommand},
  {"RENAME", renameCommand}
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