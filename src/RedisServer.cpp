#include "../inc/RedisServer.hpp"
#include "../inc/RedisCommandHandler.h"
#include "../lib/socket/socket.lib.hpp"
#include <cstring>
#include <thread>

static RedisServer *server = nullptr;
RedisServer::RedisServer(int port)
    : port(port), socket_server(SocketType::SERVER, port), running(true) {
  server = this;
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

}
void RedisServer::terminate() { socket_server.terminate(); }
