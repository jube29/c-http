#include "http_response.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const status_code_pair_t status_codes[] = {{200, "OK"},
                                           {201, "Created"},
                                           {400, "Bad Request"},
                                           {404, "Not Found"},
                                           {405, "Method Not Allowed"},
                                           {413, "Payload Too Large"},
                                           {415, "Unsupported Media Type"},
                                           {500, "Internal Server Error"},
                                           {505, "HTTP Version Not Supported"},
                                           {0, NULL}};

const char *status_code_to_reason_phrase(uint16_t status_code) {
  for (int i = 0; status_codes[i].phrase != NULL; i++) {
    if (status_codes[i].code == status_code) {
      return status_codes[i].phrase;
    }
  }
  return "Unknown";
}

uint16_t parse_result_to_status_code(parse_result_e result) {
  switch (result) {
  case PARSE_OK:
    return 200;
  case PARSE_INVALID_METHOD:
    return 405;
  case PARSE_INVALID_PATH:
    return 400;
  case PARSE_INVALID_PROTOCOL:
    return 505;
  case PARSE_MALFORMED_REQUEST_LINE:
  case PARSE_UNTERMINATED_REQUEST_LINE:
  case PARSE_MALFORMED_HEADERS:
    return 400;
  case PARSE_TOO_MANY_HEADERS:
  case PARSE_HEADERS_TOO_LARGE:
  case PARSE_HEADER_KEY_TOO_LARGE:
  case PARSE_HEADER_VALUE_TOO_LARGE:
  case PARSE_BODY_TOO_LARGE:
    return 413;
  case PARSE_CONTENT_LENGTH_INVALID:
  case PARSE_CONTENT_LENGTH_MISMATCH:
    return 400;
  case PARSE_UNSUPPORTED_CONTENT_TYPE:
    return 415;
  case PARSE_MEMORY_ERROR:
  default:
    return 500;
  }
}

void build_status_line(parse_result_e result, http_response_t *response) {
  uint16_t status_code = parse_result_to_status_code(result);
  const char *reason_phrase = status_code_to_reason_phrase(status_code);

  strcpy(response->protocol, HTTP_VERSION);
  response->status_code = status_code;
  strcpy(response->reason_phrase, reason_phrase);
}

void build_response_headers(http_response_t *response) {
  if (!response) {
    return;
  }

  size_t header_count = 3;

  response->headers = malloc(header_count * sizeof(http_header_t));
  if (!response->headers) {
    response->headers_count = 0;
    return;
  }

  strcpy(response->headers[0].key, "Connection");
  strcpy(response->headers[0].value, "close");

  strcpy(response->headers[1].key, "Content-Length");
  snprintf(response->headers[1].value, HTTP_HEADER_VALUE_LEN, "%zu", response->body_length);

  strcpy(response->headers[2].key, "Content-Type");
  strcpy(response->headers[2].value, "text/plain");

  response->headers_count = header_count;
}

parse_result_e set_response_body(http_response_t *response, const char *body) {
  if (!response) {
    return PARSE_MEMORY_ERROR;
  }

  if (response->body) {
    free(response->body);
    response->body = NULL;
    response->body_length = 0;
  }

  if (!body) {
    return PARSE_OK;
  }

  size_t body_len = strlen(body);
  if (body_len > HTTP_MAX_BODY_SIZE) {
    return PARSE_BODY_TOO_LARGE;
  }

  response->body = malloc(body_len + 1);
  if (!response->body) {
    return PARSE_MEMORY_ERROR;
  }

  strcpy(response->body, body);
  response->body_length = body_len;

  return PARSE_OK;
}

parse_result_e build_response(parse_result_e result, const char *body, http_response_t *response) {
  if (!response) {
    return PARSE_MEMORY_ERROR;
  }

  build_status_line(result, response);

  parse_result_e body_result = set_response_body(response, body);
  if (body_result != PARSE_OK) {
    return body_result;
  }

  build_response_headers(response);

  return PARSE_OK;
}

char *response_to_string(const http_response_t *response) {
  if (!response) {
    return NULL;
  }

  size_t buffer_size = HTTP_RESPONSE_BUFFER_SIZE;
  char *buffer = malloc(buffer_size);
  if (!buffer) {
    return NULL;
  }

  int len =
      snprintf(buffer, buffer_size, "%s %d %s\r\n", response->protocol, response->status_code, response->reason_phrase);

  if (len < 0 || (size_t)len >= buffer_size) {
    free(buffer);
    return NULL;
  }

  for (size_t i = 0; i < response->headers_count; i++) {
    int header_len =
        snprintf(buffer + len, buffer_size - len, "%s: %s\r\n", response->headers[i].key, response->headers[i].value);
    if (header_len < 0 || len + header_len >= (int)buffer_size) {
      free(buffer);
      return NULL;
    }
    len += header_len;
  }

  int crlf_len = snprintf(buffer + len, buffer_size - len, "\r\n");
  if (crlf_len < 0 || len + crlf_len >= (int)buffer_size) {
    free(buffer);
    return NULL;
  }
  len += crlf_len;

  if (response->body && response->body_length > 0) {
    if (len + (int)response->body_length >= (int)buffer_size) {
      free(buffer);
      return NULL;
    }
    memcpy(buffer + len, response->body, response->body_length);
    len += response->body_length;
  }

  buffer[len] = '\0';
  return buffer;
}

void free_http_response(http_response_t *response) {
  if (response->headers) {
    free(response->headers);
    response->headers = NULL;
    response->headers_count = 0;
  }
  if (response->body) {
    free(response->body);
    response->body = NULL;
    response->body_length = 0;
  }
}