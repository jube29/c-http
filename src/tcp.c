#include "tcp.h"
#include "debug.h"
#include "http_types.h"
#include "http_request.h"
#include "http_response.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


void handle_client_data(connection_manager *manager, int index) {
  client_connection *client = &manager->clients[index];
  ssize_t bytes_read = recv(client->fd, client->buffer + client->buffer_len, BUFFER_SIZE - client->buffer_len, 0);

  if (bytes_read <= 0) {
    if (bytes_read == 0 || (bytes_read == -1 && errno != EAGAIN && errno != EWOULDBLOCK)) {
      remove_client(manager, index);
    }
    return;
  }

  client->buffer_len += bytes_read;

  http_request_t request = {0};
  parse_result_e result = parse_http_request(client->buffer, &request);

  debug_log("New request: %s %s %s\n", request.method, request.path, request.protocol);

  http_response_t response = {0};
  // TODO reponse_body to be defined
  const char *response_body = "";
  parse_result_e build_result = build_response(result, response_body, &response);

  // TODO 500
  if (build_result != PARSE_OK) {
    fprintf(stderr, "Response building failed: %d\n", build_result);
    free_http_request(&request);
    remove_client(manager, index);
    return;
  }

  char *response_string = response_to_string(&response);
  // TODO 500
  if (!response_string) {
    fprintf(stderr, "Response string conversion failed\n");
    free_http_request(&request);
    free_http_response(&response);
    remove_client(manager, index);
    return;
  }

  if (send(client->fd, response_string, strlen(response_string), 0) == -1) {
    perror("Send failed");
  }
  // TODO 500

  free(response_string);
  free_http_request(&request);
  free_http_response(&response);
  remove_client(manager, index);
}

void handle_new_connection(connection_manager *manager, int server_fd) {
  int client_fd = accept_client(server_fd);
  if (client_fd != -1) {
    add_client(manager, client_fd);
  }
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
      handle_new_connection(manager, server->socket_fd);
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

