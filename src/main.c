#include "main.h"
#include "tcp.h"

#include <stdio.h>
#include <stdlib.h>

int main() {
  tcp_server server = {0};
  server_status_e status = bind_tcp_port(&server);
  if (status != SERVER_OK) {
    perror("Server initialization failed");
    exit(EXIT_FAILURE);
  }

  run_server(&server);
  return 0;
}

