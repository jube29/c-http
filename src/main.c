#include "main.h"
#include "tcp.h"
#include "connection.h"

#include <stdio.h>
#include <stdlib.h>

int main() {
  tcp_server server = {0};
  server_status_e status = bind_tcp_port(&server);
  if (status != SERVER_OK) {
    perror("Server initialization failed");
    exit(EXIT_FAILURE);
  }

  connection_manager *manager = malloc(sizeof(connection_manager));
  if (manager == NULL) {
    perror("Failed to allocate connection manager");
    exit(EXIT_FAILURE);
  }
  init_connection_manager(manager);

  run_server(&server, manager);
  
  free(manager);
  return 0;
}

