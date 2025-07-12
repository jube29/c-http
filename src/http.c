#include "http.h"

#include <stdio.h>
#include <string.h>

parse_result_e parse_http_method(const char *method) {
  if(strcmp(method, "GET") == 0) {
    return PARSE_OK;
  } else {
    return PARSE_INVALID_METHOD;
  }
}

parse_result_e parse_http_path(const char *path) {
  if (strlen(path) == 0 || path[0] != '/' || strlen(path) > HTTP_PATH_LEN) {
    return PARSE_INVALID_PATH;
  }
  if (strchr(path, ' ') != NULL) {
    return PARSE_INVALID_PATH;
  }
  return PARSE_OK;
}

parse_result_e parse_http_protocol(const char *protocol) {
  char protocol_clean[HTTP_PROTOCOL_LEN];
  strcpy(protocol_clean, protocol);

  if(strcmp(protocol_clean, "HTTP/1.1") != 0) {
    return PARSE_INVALID_PROTOCOL;
  }
  return PARSE_OK;
}

parse_result_e parse_http_request_line(const char *line, http_request_t *request) {
  char method_str[HTTP_METHOD_LEN], path_str[HTTP_PATH_LEN], protocol_str[HTTP_PROTOCOL_LEN];
  
  if (sscanf(line, "%7s %255s %15s", method_str, path_str, protocol_str) != 3) {
    return PARSE_MALFORMED_LINE;
  }

  if (parse_http_method(method_str) != PARSE_OK) {
    return PARSE_INVALID_METHOD;
  }
  strcpy(request->method, method_str);
  
  if (parse_http_path(path_str) != PARSE_OK) {
    return PARSE_INVALID_PATH;
  }
  strcpy(request->method, path_str);
  
  if (parse_http_protocol(protocol_str) != PARSE_OK) {
    return PARSE_INVALID_PROTOCOL;
  }
  strcpy(request->method, protocol_str);
  
  return PARSE_OK;
}

parse_result_e parse_http_request(const char *data, http_request_t *request) {
  const char *line_end = strstr(data, "\r\n");
  if (line_end == NULL) {
    return PARSE_MALFORMED_LINE;
  }
  
  char request_line[HTTP_REQUEST_LINE_LEN];
  size_t line_len = line_end - data;
  if (line_len >= sizeof(request_line) - 1) {
    return PARSE_MALFORMED_LINE;
  }
  
  strncpy(request_line, data, line_len);
  request_line[line_len] = '\0';
  
  parse_result_e result = parse_http_request_line(request_line, request);
  if (result != PARSE_OK) {
    return result;
  }
  
  return PARSE_OK;
}
