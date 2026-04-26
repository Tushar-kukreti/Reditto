#include "../inc/RedisServer.hpp"
#include "../lib/socket/socket.lib.hpp"
RedisServer::RedisServer(int port)
    : port(port), socket_server(SocketType::SERVER, port), running(true) {}

void RedisServer::run() { socket_server.listenClient(10); }
void RedisServer::terminate() { socket_server.terminate(); }