#include "tcp.h"
#include "debug.h"
#include "http_handler.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void handle_client_data(connection_manager *manager, int index) {
  ssize_t bytes_read = recv_client_data(manager, index);

  if (bytes_read <= 0) {
    return;
  }

  client_connection *client = &manager->clients[index];

  http_process_result_e result = process_http_buffer(client->buffer, client->buffer_len, client->fd);

  if (result == HTTP_PROCESS_ERROR) {
    fprintf(stderr, "HTTP processing failed\n");
  }

  remove_client(manager, index);
}

void run_server(tcp_server *server) {
  connection_manager *manager = malloc(sizeof(connection_manager));
  if (manager == NULL) {
    perror("Failed to allocate connection manager");
    return;
  }
  init_connection_manager(manager);

  manager->poll_fds[0].fd = server->socket_fd;
  manager->poll_fds[0].events = POLLIN;
  manager->poll_count = 1;

  debug_log("Server running and waiting for connections...\n");

  while (1) {
    int poll_result = poll(manager->poll_fds, manager->poll_count, -1);
    if (poll_result < 0) {
      perror("Poll failed");
      break;
    }

    if (manager->poll_fds[0].revents & POLLIN) {
      int client_fd = accept_client(server->socket_fd);
      if (client_fd != -1) {
        add_client(manager, client_fd);
      }
    }

    for (int i = manager->poll_count - 1; i >= 1; i--) {
      if (manager->poll_fds[i].revents & POLLIN) {
        handle_client_data(manager, i - 1);
      }
    }
  }

  for (int i = 0; i < manager->client_count; i++) {
    close(manager->clients[i].fd);
  }
  free(manager);
  close(server->socket_fd);
}

