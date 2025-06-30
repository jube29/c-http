#ifndef HTTP_H
#define HTTP_H

typedef enum {
    PARSE_OK,
    PARSE_INVALID_METHOD,
    PARSE_INVALID_PATH,
    PARSE_INVALID_PROTOCOL,
    PARSE_MALFORMED_LINE
} parse_result_e;

typedef struct {
  char method[8];
  char path[2048];
  char protocol[16];
} http_request_t;

parse_result_e parse_http_method(const char *method, char *result);
parse_result_e parse_http_path(const char *path, char *result);
parse_result_e parse_http_protocol(const char *protocol, char *result);
parse_result_e parse_http_request_line(const char *line, http_request_t *request);

#endif // HTTP_H
