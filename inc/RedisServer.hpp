#ifndef REDIS_SERVER_HPP
#define REDIS_SERVER_HPP
#include "../lib/socket/socket.lib.hpp"
#include <atomic>
class RedisServer {
public:
  RedisServer(int port);
  void run();
  void terminate();

private:
  int port;
  TCP_socket socket_server;
  std::atomic<bool> running;
};
#endif