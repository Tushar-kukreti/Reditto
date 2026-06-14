#include "../inc/RedisServer.hpp"
#include "../inc/RedisCommandHandler.hpp"
#include "../inc/RedisDatabase.hpp"
#include "../inc/constants.hpp"
#include "../lib/socket/socket.lib.hpp"
#include <cstring>
#include <thread>
#include <signal.h>

static RedisServer *globalServer = nullptr;
RedisServer::RedisServer(int port)
    : port(port), socket_server(SocketType::SERVER, port), running(true) {
  globalServer = this;
}

void signalHandler(int signum){
  if (globalServer){
    std::cout << "Caught Signal " << signum << " shutting down...\n";
    globalServer->terminate();
  }
  // Removed exit() so the main thread loop can break and perform the dump
}
void RedisServer::setupSignalHandler(){
  signal(SIGINT, signalHandler);
}
void RedisServer::listen() {
  socket_server.listenClient(10);

  std::vector<std::thread> clientThreads;
  RedisCommandHandler commandHandler;
  while (running) {
    int clientSocketFD = socket_server.acceptClient();
    if (clientSocketFD == -1) continue;

    // Handle client in seperate threads to make Redis server concurrent
    clientThreads.emplace_back([clientSocketFD, this, &commandHandler]() {
      char buff[1024];
      while (true) {
        memset(buff, 0, 1024);
        int bytesRecv = socket_server.receiveData(buff, 1024, clientSocketFD);
        if (bytesRecv == -1)
          break;
        std::string command(buff, bytesRecv);
        std::string response = commandHandler.processCommand(command);
        socket_server.sendData(response.c_str(), response.size(),
                               clientSocketFD);
      }

      // Closing the client socket to free up the resources
      socket_server.closeSocket(clientSocketFD);
    });
  }

  // Waiting for all the client threads to finish
  for (auto &t : clientThreads) {
    if (t.joinable())
      t.join();
  }

  // Before shutting down, we will dump all the data from server to db
  if (RedisDatabase::getInstance().dump(Constants::my_db))
    std::cout << "Dumped data to " << Constants::my_db << std::endl;
  else 
    std::cerr << "Unable to dump data to " << Constants::my_db << std::endl;
  }
void RedisServer::terminate() { 
  std::cout << "Terminating the server...\n";
  std::cout << "Port " << port << " is now closed for new connections.\n";
  running = false;
  socket_server.terminate();
}
