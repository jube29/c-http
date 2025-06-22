#include "http.h"
#include <string.h>

parse_result_e parse_http_request(const char *line, http_request *request) {
  char method_str[32], path_str[256], protocol_str[16];
  
  parse_result_e result = split_http_request(line, method_str, path_str, protocol_str);
  if (result != PARSE_OK) {
    return result;
  }
  
  result = parse_http_method(method_str, &request->method);
  if (result != PARSE_OK) {
    return result;
  }
  
  result = parse_http_path(path_str, request->path);
  if (result != PARSE_OK) {
    return result;
  }
  
  result = parse_http_protocol(protocol_str, request->protocol);
  if (result != PARSE_OK) {
    return result;
  }
  
  return PARSE_OK;
}

parse_result_e split_http_request_line(const char *line, char *method, char *path, char *protocol) {
  const char *method_end = strchr(line, ' ');
  if (!method_end) {
    return PARSE_MALFORMED_LINE;
  }
  
  const char *path_end = strchr(method_end + 1, ' ');
  if (!path_end) {
    return PARSE_MALFORMED_LINE;
  }
  
  size_t method_len = method_end - line;
  size_t path_len = path_end - (method_end + 1);
  size_t protocol_len = strlen(path_end + 1);

  if (method_len > 32 || path_len > 256 || protocol_len > 16) {
    return PARSE_MALFORMED_LINE;
  }
  
  strncpy(method, line, method_len);
  method[method_len] = '\0';
  strncpy(path, method_end + 1, path_len);
  path[path_len] = '\0';
  strcpy(protocol, path_end + 1);
  
  return PARSE_OK;
}

parse_result_e parse_http_method(const char *method, http_method_e *result) {
  if(strcmp(method, "GET") == 0) {
    *result = HTTP_GET;
    return PARSE_OK;
  } else if(strcmp(method, "POST") == 0) {
    *result = HTTP_POST;
    return PARSE_OK;
  } else if(strcmp(method, "PUT") == 0) {
    *result = HTTP_PUT;
    return PARSE_OK;
  } else if(strcmp(method, "DELETE") == 0) {
    *result = HTTP_DELETE;
    return PARSE_OK;
  } else if(strcmp(method, "HEAD") == 0) {
    *result = HTTP_HEAD;
    return PARSE_OK;
  } else if(strcmp(method, "OPTIONS") == 0) {
    *result = HTTP_OPTIONS;
    return PARSE_OK;
  } else {
    *result = HTTP_UNKNOWN;
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
  if(strcmp(protocol, "HTTP/1.1") != 0) {
    return PARSE_INVALID_PROTOCOL;
  }
  strcpy(result, protocol);
  return PARSE_OK;
}