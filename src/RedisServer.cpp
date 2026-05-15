#include "../inc/RedisServer.hpp"
#include "../lib/socket/socket.lib.hpp"

static RedisServer *server = nullptr;
RedisServer::RedisServer(int port)
    : port(port), socket_server(SocketType::SERVER, port), running(true) {
  server = this;
}

void RedisServer::listen() { socket_server.listenClient(10); }
void RedisServer::terminate() { socket_server.terminate(); }