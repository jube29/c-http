#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <netinet/in.h>
#include <sys/socket.h>

typedef enum { SERVER_OK = 0, SERVER_SOCKET_ERROR, SERVER_BIND_ERROR, SERVER_LISTEN_ERROR } server_status_e;

typedef struct {
  int socket_fd;
  struct sockaddr_in address;
} tcp_server;

server_status_e bind_tcp_port(tcp_server *server);
int accept_client(int server_fd);

#endif