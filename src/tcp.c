#include "tcp.h"
#include "main.h"

#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

server_status_e bind_tcp_port(tcp_server *server) {
    memset(server, 0, sizeof(*server));
    server->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->socket_fd == -1) {
        perror("Socket creation failed");
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
    printf("Server bound and listening on localhost\n");
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
        printf("Maximum number of clients reached\n");
        close(client_fd);
        return;
    }

    // Add to clients array
    manager->clients[manager->client_count].fd = client_fd;
    manager->clients[manager->client_count].buffer_len = 0;

    // Add to poll array
    manager->poll_fds[manager->poll_count].fd = client_fd;
    manager->poll_fds[manager->poll_count].events = POLLIN;
    manager->poll_fds[manager->poll_count].revents = 0;

    manager->client_count++;
    manager->poll_count++;
    printf("New client connected. Total clients: %d\n", manager->client_count);
}

void remove_client(connection_manager *manager, int index) {
    if (index < 0 || index >= manager->client_count) return;

    // Close the client socket
    close(manager->clients[index].fd);

    // Remove from poll array
    for (int i = index; i < manager->poll_count - 1; i++) {
        manager->poll_fds[i] = manager->poll_fds[i + 1];
    }

    // Remove from clients array
    for (int i = index; i < manager->client_count - 1; i++) {
        manager->clients[i] = manager->clients[i + 1];
    }

    manager->client_count--;
    manager->poll_count--;
    printf("Client disconnected. Total clients: %d\n", manager->client_count);
}

void handle_client_data(connection_manager *manager, int index) {
    client_connection *client = &manager->clients[index];
    ssize_t bytes_read = recv(client->fd, client->buffer + client->buffer_len,
                             BUFFER_SIZE - client->buffer_len, 0);

    if (bytes_read <= 0) {
        if (bytes_read == 0 || (bytes_read == -1 && errno != EAGAIN && errno != EWOULDBLOCK)) {
            remove_client(manager, index);
        }
        return;
    }

    client->buffer_len += bytes_read;
    
    // Echo back the received data
    if (send(client->fd, client->buffer, client->buffer_len, 0) == -1) {
        perror("Send failed");
        remove_client(manager, index);
        return;
    }

    // Clear the buffer after sending
    client->buffer_len = 0;
}

void handle_new_connection(connection_manager *manager, int server_fd) {
    int client_fd = accept_client(server_fd);
    if (client_fd != -1) {
        add_client(manager, client_fd);
    }
}

void run_server(tcp_server *server) {
    connection_manager manager;
    init_connection_manager(&manager);

    // Add server socket to poll array
    manager.poll_fds[0].fd = server->socket_fd;
    manager.poll_fds[0].events = POLLIN;
    manager.poll_count = 1;

    printf("Server running and waiting for connections...\n");

    while (1) {
        int poll_result = poll(manager.poll_fds, manager.poll_count, -1);
        if (poll_result < 0) {
            perror("Poll failed");
            break;
        }

        // Check server socket for new connections
        if (manager.poll_fds[0].revents & POLLIN) {
            handle_new_connection(&manager, server->socket_fd);
        }

        // Check client sockets for data
        for (int i = 1; i < manager.poll_count; i++) {
            if (manager.poll_fds[i].revents & POLLIN) {
                handle_client_data(&manager, i - 1);
            }
        }
    }

    // Cleanup
    for (int i = 0; i < manager.client_count; i++) {
        close(manager.clients[i].fd);
    }
    close(server->socket_fd);
}
