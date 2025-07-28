#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>
#define HTTP_REQUEST_LINE_LEN 4096
#define HTTP_METHOD_LEN 8
#define HTTP_PATH_LEN 2048
#define HTTP_PROTOCOL_LEN 16
#define HTTP_HEADER_KEY_LEN 256
#define HTTP_HEADER_VALUE_LEN 512
#define HTTP_MAX_HEADERS 10
#define HTTP_MAX_HEADERS_SIZE 8192

typedef enum {
  PARSE_OK,
  PARSE_MEMORY_ERROR,
  PARSE_INVALID_METHOD,
  PARSE_INVALID_PATH,
  PARSE_INVALID_PROTOCOL,
  PARSE_MALFORMED_REQUEST_LINE,
  PARSE_UNTERMINATED_REQUEST_LINE,
  PARSE_MALFORMED_HEADERS,
  PARSE_TOO_MANY_HEADERS,
  PARSE_HEADERS_TOO_LARGE,
  PARSE_HEADER_KEY_TOO_LARGE,
  PARSE_HEADER_VALUE_TOO_LARGE
} parse_result_e;

typedef struct {
  char key[HTTP_HEADER_KEY_LEN];
  char value[HTTP_HEADER_VALUE_LEN];
} http_header_t;

typedef struct {
  char method[HTTP_METHOD_LEN];
  char path[HTTP_PATH_LEN];
  char protocol[HTTP_PROTOCOL_LEN];
  http_header_t *headers;
  size_t headers_count;
} http_request_t;

parse_result_e parse_http_method(const char *method);
parse_result_e parse_http_path(const char *path);
parse_result_e parse_http_protocol(const char *protocol);
parse_result_e parse_http_request_line(const char *line, http_request_t *request);
parse_result_e parse_http_request(const char *data, http_request_t *request);
parse_result_e parse_http_headers(const char *headers, http_request_t *request);
void free_http_headers(http_request_t *request);

#endif // HTTP_H

