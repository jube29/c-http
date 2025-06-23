#ifndef TCP_H
#define TCP_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef enum {
    SERVER_OK = 0,
    SERVER_SOCKET_ERROR,
    SERVER_BIND_ERROR,
    SERVER_LISTEN_ERROR
} server_status_e;

typedef struct {
    int socket_fd;
    struct sockaddr_in address;
} tcp_server;

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

server_status_e bind_tcp_port(tcp_server *server);
int accept_client(int server_fd);
void init_connection_manager(connection_manager *manager);
void add_client(connection_manager *manager, int client_fd);
void remove_client(connection_manager *manager, int index);
void handle_client_data(connection_manager *manager, int index);
void handle_new_connection(connection_manager *manager, int server_fd);
void run_server(tcp_server *server);

#endif // TCP_H
