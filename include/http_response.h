#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include "http_types.h"

extern const status_code_pair_t status_codes[];

uint16_t parse_result_to_status_code(parse_result_e result);
const char *status_code_to_reason_phrase(uint16_t status_code);
void build_status_line(parse_result_e result, http_response_t *response);
void build_response_headers(http_response_t *response);
parse_result_e set_response_body(http_response_t *response, const char *body);
parse_result_e build_response(parse_result_e result, const char *body, http_response_t *response);
char *response_to_string(const http_response_t *response);
void free_http_response(http_response_t *response);

#endif