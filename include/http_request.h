#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "http_types.h"

parse_result_e parse_http_method(const char *method);
parse_result_e parse_http_path(const char *path);
parse_result_e parse_http_protocol(const char *protocol);
parse_result_e parse_http_request_line(const char *line, http_request_t *request);
parse_result_e parse_http_request(const char *data, http_request_t *request);
parse_result_e parse_http_headers(const char *headers, http_request_t *request);
parse_result_e parse_http_body(const char *data, size_t data_length, http_request_t *request);
const char *get_header_value(const http_request_t *request, const char *key);
void free_http_headers(http_request_t *request);
void free_http_body(http_request_t *request);
void free_http_request(http_request_t *request);

#endif