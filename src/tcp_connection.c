#include "tcp_connection.h"
#include "debug.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void init_connection_manager(connection_manager *manager) {
  memset(manager, 0, sizeof(*manager));
  manager->poll_count = 0;
}

void add_client(connection_manager *manager, int client_fd) {
  if (manager->client_count >= MAX_CLIENTS) {
    fprintf(stderr, "Maximum number of clients reached\n");
    close(client_fd);
    return;
  }

  manager->clients[manager->client_count].fd = client_fd;
  manager->clients[manager->client_count].buffer_len = 0;

  manager->poll_fds[manager->poll_count].fd = client_fd;
  manager->poll_fds[manager->poll_count].events = POLLIN;
  manager->poll_fds[manager->poll_count].revents = 0;

  manager->client_count++;
  manager->poll_count++;
  debug_log("New client connected. Total clients: %d\n", manager->client_count);
}

void remove_client(connection_manager *manager, int index) {
  if (index < 0 || index >= manager->client_count)
    return;

  memset(manager->clients[index].buffer, 0, BUFFER_SIZE);
  manager->clients[index].buffer_len = 0;

  close(manager->clients[index].fd);

  for (int i = index; i < manager->poll_count - 1; i++) {
    manager->poll_fds[i] = manager->poll_fds[i + 1];
  }

  for (int i = index; i < manager->client_count - 1; i++) {
    manager->clients[i] = manager->clients[i + 1];
  }

  manager->client_count--;
  manager->poll_count--;
  debug_log("Client disconnected. Total clients: %d\n", manager->client_count);
}

ssize_t recv_client_data(connection_manager *manager, int index) {
  if (index < 0 || index >= manager->client_count)
    return -1;

  client_connection *client = &manager->clients[index];
  ssize_t bytes_read = recv(client->fd, client->buffer + client->buffer_len, BUFFER_SIZE - client->buffer_len, 0);

  if (bytes_read <= 0) {
    if (bytes_read == 0 || (bytes_read == -1 && errno != EAGAIN && errno != EWOULDBLOCK)) {
      remove_client(manager, index);
    }
    return bytes_read;
  }

  client->buffer_len += bytes_read;
  return bytes_read;
}

