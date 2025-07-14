#ifndef HTTP_H
#define HTTP_H

#define HTTP_REQUEST_LINE_LEN 4096
#define HTTP_METHOD_LEN 8
#define HTTP_PATH_LEN 2048
#define HTTP_PROTOCOL_LEN 16

typedef enum {
    PARSE_OK,
    PARSE_INVALID_METHOD,
    PARSE_INVALID_PATH,
    PARSE_INVALID_PROTOCOL,
    PARSE_MALFORMED_REQUEST_LINE,
    PARSE_UNTERMINATED_REQUEST_LINE
} parse_result_e;

typedef struct {
  char method[HTTP_METHOD_LEN];
  char path[HTTP_PATH_LEN];
  char protocol[HTTP_PROTOCOL_LEN];
} http_request_t;

parse_result_e parse_http_method(const char *method);
parse_result_e parse_http_path(const char *path);
parse_result_e parse_http_protocol(const char *protocol);
parse_result_e parse_http_request_line(const char *line, http_request_t *request);
parse_result_e parse_http_request(const char *data, http_request_t *request);

#endif // HTTP_H
