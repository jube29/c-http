#ifndef TCP_H
#define TCP_H

#include <netinet/in.h>
#include <sys/socket.h>
#include "tcp_server.h"
#include "tcp_connection.h"

void handle_client_data(connection_manager *manager, int index);
void run_server(tcp_server *server);

#endif

