#include "../inc/RedisCommandHandler.h"
#include <algorithm>
#include <cctype>
#include <sstream>

std::vector<std::string> parseCommand(std::string command) {
  int pos = 0;
  std::vector<std::string> response;

  // parse with spacing if not in resp formate
  if (command[0] != '*') {
    std::ostringstream input(command);
    std::string token;
    while (input << token)
      response.push_back(token);
    return response;
  }

  // parse the resp array
  if (command[pos] != '*')
    return response;
  pos++;
  int crlf = command.find("\r\n");
  int num_token = stoi(command.substr(pos, crlf - pos));
  pos = crlf + 2;
  for (int i = 0; i < num_token; i++) {
    pos++;
    crlf = command.find("\r\n", pos);
    int len = stoi(command.substr(pos, crlf - pos));
    pos = crlf + 2;

    std::string token = "";
    crlf = command.find("\r\n", pos);
    token = command.substr(pos, len);
    response.push_back(token);
    pos = crlf + 2;
  }
  return response;
}

RedisCommandHandler::RedisCommandHandler() {}
RedisCommandHandler::~RedisCommandHandler() {}

std::string RedisCommandHandler::processCommand(std::string command) {
  std::vector<std::string> tokens = parseCommand(command);
  if (tokens.size() == 0)
    return "-Error Invalid Command\r\n";
  std::string cmd = tokens[0];
  std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
  std::ostringstream response;

  // check to database
  // check command type
  
  return "+OK\r\n"; // Temporary valid RESP response to prevent client hang
}