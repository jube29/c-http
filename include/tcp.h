#ifndef TCP_H
#define TCP_H

#include <netinet/in.h>
#include <sys/socket.h>
#include "server.h"
#include "connection.h"

void handle_client_data(connection_manager *manager, int index);
void run_server(tcp_server *server, connection_manager *manager);

#endif

