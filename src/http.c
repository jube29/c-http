#include "http.h"

#include <stdio.h>
#include <string.h>

parse_result_e parse_http_method(const char *method, char *result) {
  if(strcmp(method, "GET") == 0) {
    strcpy(result, "GET");
    return PARSE_OK;
  } else {
    return PARSE_INVALID_METHOD;
  }
}

parse_result_e parse_http_path(const char *path, char *result) {
  if (strlen(path) == 0 || path[0] != '/' || strlen(path) > 256) {
    return PARSE_INVALID_PATH;
  }
  if (strchr(path, ' ') != NULL) {
    return PARSE_INVALID_PATH;
  }
  strcpy(result, path);
  return PARSE_OK;
}

parse_result_e parse_http_protocol(const char *protocol, char *result) {
  char protocol_clean[16];
  strcpy(protocol_clean, protocol);
  char *cr = strchr(protocol_clean, '\r');
  if (cr) *cr = '\0';
  char *lf = strchr(protocol_clean, '\n');
  if (lf) *lf = '\0';

  if(strcmp(protocol_clean, "HTTP/1.1") != 0) {
    return PARSE_INVALID_PROTOCOL;
  }
  strcpy(result, protocol_clean);
  return PARSE_OK;
}

parse_result_e parse_http_request_line(const char *line, http_request *request) {
  char method_str[8], path_str[256], protocol_str[16];
  
  if (sscanf(line, "%7s %255s %15s", method_str, path_str, protocol_str) != 3) {
    return PARSE_MALFORMED_LINE;
  }

  if (parse_http_method(method_str, request->method) != PARSE_OK) {
    return PARSE_INVALID_METHOD;
  }
  
  if (parse_http_path(path_str, request->path) != PARSE_OK) {
    return PARSE_INVALID_PATH;
  }
  
  if (parse_http_protocol(protocol_str, request->protocol) != PARSE_OK) {
    return PARSE_INVALID_PROTOCOL;
  }
  
  return PARSE_OK;
}
