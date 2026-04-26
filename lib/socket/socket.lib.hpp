#ifndef SOCKET_LIB_HPP
#define SOCKET_LIB_HPP
#include <arpa/inet.h> // inet_pton()
#include <iostream>
#include <netinet/in.h> // sockaddr, sockaddr_in, sockaddr_in6
#include <sys/socket.h> // socket(), connect(), bind(), listen(), accept()
#include <unistd.h>     // close
enum class SocketType { SERVER, CLIENT };

class TCP_socket {
private:
  int socketFD;
  int port;

public:
  // TCP Socket Constructor
  TCP_socket(SocketType type, int port = 6396) {
    // AF_INET 		- IPV4
    // SOCK_STREAM 	- TCP Connection
    // 0			- Defines the protocol, 0 means we want IP
    // protocol
    this->port = port;
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0) {
      std::cerr << "ERROR:: Socket Creation Failed.\n";
    } else {
      std::cout << "Socket created successfully.\n";
    }
    // in standard socket connection OS adds a time-delay of 60 sec, after
    // socket connection termination to make sure that all the packets are
    // transfered correctly and no data is lost. to turn off this feature and
    // reuse the port immediately after termination we use SO_REUSEADDR.
    int opt = 1;
    if (setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
      std::cerr << "ERROR:: setsockopt(SO_REUSEADDR) failed.\n";
    }

    if (type == SocketType::SERVER) {
      // server
      struct sockaddr_in address = getSocketAddress("", port);
      if (bind(socketFD, (struct sockaddr *)&address, sizeof(address)) != 0) {
        std::cerr << "ERROR:: Socket Binding Failed.\n";
      } else {
        std::cout << "Socket bound successfully.\n";
      }
    }
  }
  // Terminate Method for the util
  void terminate() {
    shutdown(socketFD, SHUT_RDWR);
    close(socketFD);
    std::cout << "Socket closed successfully.\n";
  }
  // Destructor
  ~TCP_socket() { terminate(); }
  struct sockaddr_in getSocketAddress(const char *ip, int port) {
    struct sockaddr_in address;
    address.sin_port = htons(port);
    address.sin_family = AF_INET;
    if (strlen(ip) == 0)
      address.sin_addr.s_addr = INADDR_ANY;
    else
      inet_pton(AF_INET, ip, &address.sin_addr.s_addr);
    return address;
  }
  // Method to connect a client to a server socket
  int connectToIPV4(const char *ip, int port) {
    struct sockaddr_in address = getSocketAddress(ip, port);
    return connect(socketFD, (struct sockaddr *)&address, sizeof(address));
  }
  // Method to listen for a incomming clients
  void listenClient(int backlog = 10) {
    if (listen(socketFD, backlog) < 0) {
      std::cerr << "ERROR:: Socket Listening Failed.\n";
    } else {
      std::cout << "Socket listening on port " << port << ".\n";
    }
  }
  // Method to send data
  int sendData(const char *data) {
    return send(socketFD, data, strlen(data), 0);
  }
  // Method to receive Data
  int receiveData(char *buff, int sz) { return recv(socketFD, buff, sz, 0); }
  // To Return the socket fd
  int getSocketFD() { return socketFD; }
};
#endif