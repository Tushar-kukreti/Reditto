#include "../inc/RedisServer.hpp"
#include <chrono>
#include <string>
#include <thread>

int main(int argc, char **argv) {
  int port = 6379;
  if (argc > 1) {
    port = std::stoi(argv[1]);
  }
  RedisServer server(port);
  std::thread persistent_thread([]() {
    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(300));
      // TODO: persist the data to disk every 30 mins
    }
  });
  persistent_thread.detach();
  server.listen();
  return 0;
}