#ifndef HTTP_H
#define HTTP_H

typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_HEAD,
    HTTP_OPTIONS,
    HTTP_UNKNOWN
} http_method_e;

typedef enum {
    PARSE_OK,
    PARSE_INVALID_METHOD,
    PARSE_INVALID_PATH,
    PARSE_INVALID_PROTOCOL,
    PARSE_MALFORMED_LINE
} parse_result_e;

typedef struct {
  http_method_e method;
  char path[256];
  char protocol[16];
} http_request;

parse_result_e split_http_request_line(const char *line, char *method, char *path, char *protocol);
parse_result_e parse_http_method(const char *method, http_method_e *result);
parse_result_e parse_http_path(const char *path, char *result);
parse_result_e parse_http_protocol(const char *protocol, char *result);
parse_result_e parse_http_request(const char *line, http_request *request);

#endif // HTTP_H
