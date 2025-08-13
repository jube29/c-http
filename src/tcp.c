#include "tcp.h"
#include "debug.h"
#include "http.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
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

int accept_client(int server_fd) {
  struct sockaddr_in client_address = {0};
  socklen_t client_len = sizeof(client_address);
  int client_fd = accept(server_fd, (struct sockaddr *)&client_address, &client_len);
  if (client_fd < 0) {
    perror("Accept failed");
    return -1;
  }
  return client_fd;
}

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

