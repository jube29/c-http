#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include <stddef.h>

typedef enum { HTTP_PROCESS_OK, HTTP_PROCESS_ERROR, HTTP_PROCESS_INCOMPLETE } http_process_result_e;

http_process_result_e process_http_buffer(const char *buffer, size_t buffer_len, int client_fd);

#endif

