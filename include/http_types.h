#ifndef HTTP_TYPES_H
#define HTTP_TYPES_H

#include <stddef.h>
#include <stdint.h>

#define HTTP_VERSION "HTTP/1.0"
#define HTTP_REQUEST_LINE_LEN 4096
#define HTTP_METHOD_LEN 8
#define HTTP_PATH_LEN 2048
#define HTTP_PROTOCOL_LEN 16
#define HTTP_HEADER_KEY_LEN 256
#define HTTP_HEADER_VALUE_LEN 512
#define HTTP_MAX_HEADERS 10
#define HTTP_MAX_HEADERS_SIZE 8192
#define HTTP_MAX_BODY_SIZE 1048576
#define HTTP_RESPONSE_REASON_LEN 64
#define HTTP_RESPONSE_BUFFER_SIZE (HTTP_MAX_HEADERS_SIZE + HTTP_MAX_BODY_SIZE + 1024)

typedef enum {
  PARSE_OK = 0,
  PARSE_MEMORY_ERROR = 1,
  PARSE_INVALID_METHOD = 2,
  PARSE_INVALID_PATH = 3,
  PARSE_INVALID_PROTOCOL = 4,
  PARSE_MALFORMED_REQUEST_LINE = 5,
  PARSE_UNTERMINATED_REQUEST_LINE = 6,
  PARSE_MALFORMED_HEADERS = 7,
  PARSE_TOO_MANY_HEADERS = 8,
  PARSE_HEADERS_TOO_LARGE = 9,
  PARSE_HEADER_KEY_TOO_LARGE = 10,
  PARSE_HEADER_VALUE_TOO_LARGE = 11,
  PARSE_BODY_TOO_LARGE = 12,
  PARSE_CONTENT_LENGTH_INVALID = 13,
  PARSE_CONTENT_LENGTH_MISMATCH = 14,
  PARSE_UNSUPPORTED_CONTENT_TYPE = 15,
} parse_result_e;

typedef struct {
  uint16_t code;
  const char *phrase;
} status_code_pair_t;

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
  char *body;
  size_t body_length;
} http_request_t;

typedef struct {
  char protocol[HTTP_PROTOCOL_LEN];
  uint16_t status_code;
  char reason_phrase[HTTP_RESPONSE_REASON_LEN];
  http_header_t *headers;
  size_t headers_count;
  char *body;
  size_t body_length;
} http_response_t;

#endif