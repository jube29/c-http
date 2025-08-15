#include "http_handler.h"
#include "debug.h"
#include "http_request.h"
#include "http_response.h"
#include "http_types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

http_process_result_e process_http_buffer(const char *buffer, size_t buffer_len, int client_fd) {
  http_request_t request = {0};
  parse_result_e result = parse_http_request(buffer, &request);

  debug_log("New request: %s %s %s\n", request.method, request.path, request.protocol);

  http_response_t response = {0};
  const char *response_body = "";
  parse_result_e build_result = build_response(result, response_body, &response);

  if (build_result != PARSE_OK) {
    fprintf(stderr, "Response building failed: %d\n", build_result);
    free_http_request(&request);
    return HTTP_PROCESS_ERROR;
  }

  char *response_string = response_to_string(&response);
  if (!response_string) {
    fprintf(stderr, "Response string conversion failed\n");
    free_http_request(&request);
    free_http_response(&response);
    return HTTP_PROCESS_ERROR;
  }

  if (send(client_fd, response_string, strlen(response_string), 0) == -1) {
    perror("Send failed");
    free(response_string);
    free_http_request(&request);
    free_http_response(&response);
    return HTTP_PROCESS_ERROR;
  }

  free(response_string);
  free_http_request(&request);
  free_http_response(&response);
  return HTTP_PROCESS_OK;
}