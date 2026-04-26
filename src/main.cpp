#include "../inc/RedisServer.hpp"
#include <string>
int main(int argc, char **argv) {
  int port = 6396;
  if (argc > 1) {
    port = std::stoi(argv[1]);
  }
  RedisServer server(port);
  return 0;
}