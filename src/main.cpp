#include "../inc/RedisServer.hpp"
#include "../inc/constants.hpp"
#include "../inc/constants.hpp"
#include "../inc/RedisDatabase.hpp"
#include <chrono>
#include <string>
#include <thread>

int main(int argc, char **argv) {
  int port = Constants::server_port;
  if (argc > 1) {
    port = std::stoi(argv[1]);
  }
  RedisServer server(port);
  std::thread persistent_thread([]() {
    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(300));
      if (RedisDatabase::getInstance().dump(Constants::my_db))
        std::cout << "Dumped data to " << Constants::my_db << std::endl;
      else 
        std::cerr << "ERROR:: Unable to update data in " << Constants::my_db << std::endl;
    }
  });
  persistent_thread.detach();
  server.listen();
  return 0;
}