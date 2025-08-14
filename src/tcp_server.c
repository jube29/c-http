#include "tcp_server.h"
#include "debug.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

server_status_e bind_tcp_port(tcp_server *server) {
  memset(server, 0, sizeof(*server));
  server->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server->socket_fd == -1) {
    perror("Socket creation failed");
    return SERVER_SOCKET_ERROR;
  }

  int opt = 1;
  if (setsockopt(server->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt(SO_REUSEADDR) failed");
    close(server->socket_fd);
    return SERVER_SOCKET_ERROR;
  }

  server->address.sin_family = AF_INET;
  server->address.sin_addr.s_addr = inet_addr("127.0.0.1");
  server->address.sin_port = htons(8080);
  if (bind(server->socket_fd, (struct sockaddr *)&server->address, sizeof(server->address)) < 0) {
    perror("Bind failed");
    close(server->socket_fd);
    return SERVER_BIND_ERROR;
  }
  if (listen(server->socket_fd, 5) < 0) {
    perror("Listen failed");
    close(server->socket_fd);
    return SERVER_LISTEN_ERROR;
  }
  debug_log("Server bound and listening on localhost\n");
  return SERVER_OK;
}