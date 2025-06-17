#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Common constants
#define PORT 8080
#define BUFFER_SIZE 1024

// Function declarations
int create_server_socket(void);
void handle_client(int client_socket);

#endif // MAIN_H 