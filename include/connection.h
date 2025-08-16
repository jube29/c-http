#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H

#include <poll.h>
#include <stddef.h>
#include <sys/types.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1500000

typedef struct {
  int fd;
  char buffer[BUFFER_SIZE];
  size_t buffer_len;
} client_connection;

typedef struct {
  client_connection clients[MAX_CLIENTS];
  int client_count;
  struct pollfd poll_fds[MAX_CLIENTS + 1];
  int poll_count;
} connection_manager;

void init_connection_manager(connection_manager *manager);
void add_client(connection_manager *manager, int client_fd);
void remove_client(connection_manager *manager, int index);
ssize_t recv_client_data(connection_manager *manager, int index);

#endif