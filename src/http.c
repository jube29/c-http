#include "http.h"

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

parse_result_e parse_http_method(const char *method) {
  if (strcmp(method, "GET") == 0 || strcmp(method, "POST") == 0 || strcmp(method, "HEAD") == 0) {
    return PARSE_OK;
  } else {
    return PARSE_INVALID_METHOD;
  }
}

parse_result_e parse_http_path(const char *path) {
  if (strlen(path) == 0 || path[0] != '/' || strlen(path) > HTTP_PATH_LEN - 1) {
    return PARSE_INVALID_PATH;
  }
  if (strchr(path, ' ') != NULL) {
    return PARSE_INVALID_PATH;
  }
  return PARSE_OK;
}

parse_result_e parse_http_protocol(const char *protocol) {
  if (strcmp(protocol, HTTP_VERSION) != 0) {
    return PARSE_INVALID_PROTOCOL;
  }
  return PARSE_OK;
}

parse_result_e parse_http_request_line(const char *line, http_request_t *request) {
  char method_str[HTTP_METHOD_LEN], path_str[HTTP_PATH_LEN], protocol_str[HTTP_PROTOCOL_LEN];

  if (sscanf(line, "%7s %255s %15s", method_str, path_str, protocol_str) != 3) {
    return PARSE_MALFORMED_REQUEST_LINE;
  }

  parse_result_e method_parse_result = parse_http_method(method_str);
  if (method_parse_result != PARSE_OK) {
    return method_parse_result;
  }
  strcpy(request->method, method_str);

  parse_result_e path_parse_result = parse_http_path(path_str);
  if (path_parse_result != PARSE_OK) {
    return path_parse_result;
  }
  strcpy(request->path, path_str);

  parse_result_e protocol_parse_result = parse_http_protocol(protocol_str);
  if (protocol_parse_result != PARSE_OK) {
    return protocol_parse_result;
  }
  strcpy(request->protocol, protocol_str);

  return PARSE_OK;
}

parse_result_e parse_http_request(const char *data, http_request_t *request) {
  const char *line_end = strstr(data, "\r\n");
  if (line_end == NULL) {
    return PARSE_UNTERMINATED_REQUEST_LINE;
  }

  char request_line[HTTP_REQUEST_LINE_LEN];
  size_t line_len = line_end - data;
  if (line_len >= sizeof(request_line) - 1) {
    return PARSE_MALFORMED_REQUEST_LINE;
  }

  strncpy(request_line, data, line_len);
  request_line[line_len] = '\0';

  parse_result_e result = parse_http_request_line(request_line, request);
  if (result != PARSE_OK) {
    return result;
  }

  const char *headers_start = line_end + 2;
  const char *headers_end = headers_start;
  while (headers_end && *headers_end) {
    if (strncmp(headers_end, "\r\n", 2) == 0) {
      break;
    }
    const char *line_end = strstr(headers_end, "\r\n");
    if (line_end == NULL) {
      return PARSE_MALFORMED_HEADERS;
    }
    headers_end = line_end + 2;
  }

  size_t headers_len = headers_end - headers_start;
  if (headers_len > HTTP_MAX_HEADERS_SIZE) {
    return PARSE_HEADERS_TOO_LARGE;
  }

  result = parse_http_headers(headers_start, request);
  if (result != PARSE_OK) {
    return result;
  }

  const char *body_start = headers_end + 2;

  size_t actual_body_length = strlen(body_start);

  const char *content_type_str = get_header_value(request, "Content-Type");
  if (strcmp(request->method, "POST") == 0) {
    if (content_type_str == NULL) {
      return PARSE_UNSUPPORTED_CONTENT_TYPE;
    }
    if (strcmp(content_type_str, "text/plain") != 0) {
      return PARSE_UNSUPPORTED_CONTENT_TYPE;
    }
  } else if (content_type_str != NULL) {
    if (strcmp(content_type_str, "text/plain") != 0) {
      return PARSE_UNSUPPORTED_CONTENT_TYPE;
    }
  }

  const char *content_length_str = get_header_value(request, "Content-Length");
  if (content_length_str != NULL) {
    char *endptr;
    long content_length = strtol(content_length_str, &endptr, 10);

    if (*endptr != '\0' || content_length < 0) {
      return PARSE_CONTENT_LENGTH_INVALID;
    }

    if ((size_t)content_length != actual_body_length) {
      return PARSE_CONTENT_LENGTH_MISMATCH;
    }
  }

  if (actual_body_length > HTTP_MAX_BODY_SIZE) {
    return PARSE_BODY_TOO_LARGE;
  }

  result = parse_http_body(body_start, actual_body_length, request);
  if (result != PARSE_OK) {
    return result;
  }

  return PARSE_OK;
}

parse_result_e parse_http_headers(const char *headers, http_request_t *request) {
  request->headers = NULL;
  request->headers_count = 0;

  if (strlen(headers) == 2 && strncmp(headers, "\r\n", 2) == 0) {
    return PARSE_OK;
  }

  const char *ptr = headers;
  size_t count = 0;
  while (ptr && *ptr) {
    const char *line_end = strstr(ptr, "\r\n");
    if (line_end == NULL)
      break;

    if (strlen(ptr) > 0)
      count++;

    if (count > HTTP_MAX_HEADERS) {
      return PARSE_TOO_MANY_HEADERS;
    }

    ptr = line_end + 2;
  }

  request->headers = malloc(count * sizeof(http_header_t));
  if (request->headers == NULL) {
    return PARSE_MEMORY_ERROR;
  }

  ptr = headers;
  size_t header_idx = 0;
  while (ptr && *ptr && header_idx < count) {
    const char *line_end = strstr(ptr, "\r\n");
    if (line_end == NULL)
      break;

    size_t line_len = line_end - ptr;
    if (line_len == 0) {
      break;
    }

    char line[line_len + 1];
    strncpy(line, ptr, line_len);
    line[line_len] = '\0';

    char *colon = strchr(line, ':');
    if (colon == NULL) {
      free(request->headers);
      request->headers = NULL;
      return PARSE_MALFORMED_HEADERS;
    }
    *colon = '\0';
    char *key = line;
    char *value = colon + 1;

    while (*key == ' ')
      key++;
    while (*value == ' ')
      value++;

    char *key_end = key + strlen(key) - 1;
    while (key_end > key && *key_end == ' ') {
      *key_end = '\0';
      key_end--;
    }
    char *value_end = value + strlen(value) - 1;
    while (value_end > value && *value_end == ' ') {
      *value_end = '\0';
      value_end--;
    }

    if (strlen(key) >= HTTP_HEADER_KEY_LEN) {
      free(request->headers);
      request->headers = NULL;
      return PARSE_HEADER_KEY_TOO_LARGE;
    }
    if (strlen(value) >= HTTP_HEADER_VALUE_LEN) {
      free(request->headers);
      request->headers = NULL;
      return PARSE_HEADER_VALUE_TOO_LARGE;
    }

    strcpy(request->headers[header_idx].key, key);
    strcpy(request->headers[header_idx].value, value);
    header_idx++;

    ptr = line_end + 2;
  }

  request->headers_count = header_idx;
  return PARSE_OK;
}

parse_result_e parse_http_body(const char *data, size_t data_length, http_request_t *request) {
  if (data_length > 0) {
    request->body = malloc(data_length + 1);
    if (request->body == NULL) {
      return PARSE_MEMORY_ERROR;
    }

    memcpy(request->body, data, data_length);
    request->body[data_length] = '\0';
    request->body_length = data_length;
  } else {
    request->body = NULL;
    request->body_length = 0;
  }

  return PARSE_OK;
}

const char *get_header_value(const http_request_t *request, const char *key) {
  if (!request || !request->headers || !key) {
    return NULL;
  }

  for (size_t i = 0; i < request->headers_count; i++) {
    if (strcasecmp(key, request->headers[i].key) == 0) {
      return request->headers[i].value;
    }
  }

  return NULL;
}

void free_http_headers(http_request_t *request) {
  if (request->headers) {
    free(request->headers);
    request->headers = NULL;
    request->headers_count = 0;
  }
}

void free_http_body(http_request_t *request) {
  if (request->body) {
    free(request->body);
    request->body = NULL;
    request->body_length = 0;
  }
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

void free_http_request(http_request_t *request) {
  free_http_headers(request);
  free_http_body(request);
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

