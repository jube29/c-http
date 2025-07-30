#include "http.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

parse_result_e parse_http_method(const char *method) {
  if (strcmp(method, "GET") == 0 || strcmp(method, "POST") == 0 || strcmp(method, "DELETE") == 0) {
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
  if (strcmp(protocol, "HTTP/1.1") != 0) {
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
    // Defensive: can't store this request line safely
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

  // Count the actual body length
  size_t actual_body_length = strlen(body_start);

  // Check Content-Length header if present
  const char *content_length_str = get_header_value(request, "Content-Length");
  if (content_length_str != NULL) {
    // Parse Content-Length value
    char *endptr;
    long content_length = strtol(content_length_str, &endptr, 10);

    // Check if parsing was successful and value is valid
    if (*endptr != '\0' || content_length < 0) {
      return PARSE_CONTENT_LENGTH_INVALID;
    }

    // Compare actual body length with declared Content-Length
    if ((size_t)content_length != actual_body_length) {
      return PARSE_CONTENT_LENGTH_MISMATCH;
    }
  }

  // Check if body size is within limits
  if (actual_body_length > HTTP_MAX_BODY_SIZE) {
    return PARSE_BODY_TOO_LARGE;
  }

  // Parse the body using the dedicated function
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

  // Count headers first
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

  // Allocate memory for headers
  request->headers = malloc(count * sizeof(http_header_t));
  if (request->headers == NULL) {
    return PARSE_MEMORY_ERROR;
  }

  // Parse each header
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

    // Find colon separator
    char *colon = strchr(line, ':');
    if (colon == NULL) {
      free(request->headers);
      request->headers = NULL;
      return PARSE_MALFORMED_HEADERS;
    }
    *colon = '\0';
    char *key = line;
    char *value = colon + 1;

    // Trim leading spaces
    while (*key == ' ')
      key++;
    while (*value == ' ')
      value++;

    // Trim trailing spaces
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

    // Check lengths
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
  // Dynamically allocate memory for the body if it exists
  if (data_length > 0) {
    request->body = malloc(data_length + 1); // +1 for null terminator
    if (request->body == NULL) {
      return PARSE_MEMORY_ERROR;
    }

    // Copy the body data
    memcpy(request->body, data, data_length);
    request->body[data_length] = '\0'; // Null terminate for safety
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

void free_http_request(http_request_t *request) {
  free_http_headers(request);
  free_http_body(request);
}

