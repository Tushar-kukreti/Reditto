#ifndef REDIS_COMMAND_HANDLER_H
#define REDIS_COMMAND_HANDLER_H

#include <iostream>
#include <string>
#include <vector>

class RedisCommandHandler {
public:
  RedisCommandHandler();
  ~RedisCommandHandler();
  std::string processCommand(std::string command);
};

#endif