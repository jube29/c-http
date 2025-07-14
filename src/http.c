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
  if (strlen(path) == 0 || path[0] != '/' || strlen(path) > HTTP_PATH_LEN - 1) {
    return PARSE_INVALID_PATH;
  }
  if (strchr(path, ' ') != NULL) {
    return PARSE_INVALID_PATH;
  }
  return PARSE_OK;
}

parse_result_e parse_http_protocol(const char *protocol) {
  if(strcmp(protocol, "HTTP/1.1") != 0) {
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
  if (path_parse_result  != PARSE_OK) {
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
  
  return PARSE_OK;
}
