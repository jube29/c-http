#include "../include/http.h"
#include <criterion/internal/test.h>
#include <string.h>

// Method Parsing

Test(http, should_parse_get) { cr_assert(parse_http_method("GET") == PARSE_OK, "GET method should be parsed"); }

// Protocol Parsing

Test(http, should_parse_protocol_http_1_1) {
  cr_assert(parse_http_protocol("HTTP/1.1") == PARSE_OK, "HTTP/1.1 protocol should be parsed");
}

// Path Parsing

Test(http, should_parse_valid_basic_path) {
  cr_assert(parse_http_path("/foo.html") == PARSE_OK, "Valid path should be parsed");
}

Test(http, should_not_parse_empty_path) {
  cr_assert(parse_http_path("") == PARSE_INVALID_PATH, "Empty path should not be parsed");
}

Test(http, should_not_parse_path_without_starting_slash) {
  cr_assert(parse_http_path("ar.html") == PARSE_INVALID_PATH, "Path without starting slash should not be parsed");
}

Test(http, should_not_parse_too_long_path) {
  char path[2049];
  path[0] = '/';
  memset(path + 1, 'a', 2047);
  path[2048] = '\0';
  cr_assert(parse_http_path(path) == PARSE_INVALID_PATH, "Too long path should not be parsed");
}

Test(http, should_not_parse_path_with_space_in) {
  cr_assert(parse_http_path("bar html") == PARSE_INVALID_PATH, "Path with space in should not be parsed");
}

// Request Line Parsing

Test(http, should_parse_valid_request_line) {
  http_request_t request = {0};
  cr_assert(parse_http_request_line("GET /index.html HTTP/1.1", &request) == PARSE_OK,
            "Valid request line should be parsed");
  cr_assert_str_eq(request.method, "GET", "Method should be GET");
  cr_assert_str_eq(request.path, "/index.html", "Path should be /index.html");
  cr_assert_str_eq(request.protocol, "HTTP/1.1", "Protocol should be HTTP/1.1");
}

Test(http, should_not_parse_malformed_request_line_missing_parts) {
  http_request_t request = {0};
  cr_assert(parse_http_request_line("GET /index.html", &request) == PARSE_MALFORMED_REQUEST_LINE,
            "Request line missing protocol should not be parsed");
  cr_assert(parse_http_request_line("GET", &request) == PARSE_MALFORMED_REQUEST_LINE,
            "Request line missing path and protocol should not be parsed");
  cr_assert(parse_http_request_line("", &request) == PARSE_MALFORMED_REQUEST_LINE,
            "Empty request line should not be parsed");
}

Test(http, should_not_parse_request_line_with_invalid_method) {
  http_request_t request = {0};
  cr_assert(parse_http_request_line("POST /index.html HTTP/1.1", &request) == PARSE_INVALID_METHOD,
            "Request line with invalid method should not be parsed");
  cr_assert(parse_http_request_line("DELETE /index.html HTTP/1.1", &request) == PARSE_INVALID_METHOD,
            "Request line with invalid method should not be parsed");
}

Test(http, should_not_parse_request_line_with_invalid_path) {
  http_request_t request = {0};
  cr_assert(parse_http_request_line("GET index.html HTTP/1.1", &request) == PARSE_INVALID_PATH,
            "Request line with path missing leading slash should not be parsed");
  cr_assert(parse_http_request_line("GET \"\" HTTP/1.1", &request) == PARSE_INVALID_PATH,
            "Request line with empty path should not be parsed");
}

Test(http, should_not_parse_request_line_with_invalid_protocol) {
  http_request_t request = {0};
  cr_assert(parse_http_request_line("GET /index.html HTTP/1.0", &request) == PARSE_INVALID_PROTOCOL,
            "Request line with invalid protocol should not be parsed");
  cr_assert(parse_http_request_line("GET /index.html HTTP/2.0", &request) == PARSE_INVALID_PROTOCOL,
            "Request line with invalid protocol should not be parsed");
  cr_assert(parse_http_request_line("GET /index.html HTTPS/1.1", &request) == PARSE_INVALID_PROTOCOL,
            "Request line with invalid protocol should not be parsed");
}

Test(http, should_parse_valid_full_http_request) {
  http_request_t request = {0};
  const char *http_data = "GET /test.html HTTP/1.1\r\nHost: example.com\r\n\r\n";
  cr_assert(parse_http_request(http_data, &request) == PARSE_OK, "Valid HTTP request should be parsed");
  cr_assert_str_eq(request.method, "GET", "Method should be GET");
  cr_assert_str_eq(request.path, "/test.html", "Path should be /test.html");
  cr_assert_str_eq(request.protocol, "HTTP/1.1", "Protocol should be HTTP/1.1");
}

Test(http, should_not_parse_http_request_without_crlf) {
  http_request_t request = {0};
  const char *http_data = "GET /test.html HTTP/1.1";
  cr_assert(parse_http_request(http_data, &request) == PARSE_UNTERMINATED_REQUEST_LINE,
            "HTTP request without CRLF should not be parsed");
}

Test(http, should_not_parse_http_request_with_too_long_request_line) {
  http_request_t request = {0};
  char long_line[4110];
  strcpy(long_line, "GET /");
  memset(long_line + 5, 'a', 4090);
  strcat(long_line, " HTTP/1.1\r\n");
  cr_assert(parse_http_request(long_line, &request) == PARSE_MALFORMED_REQUEST_LINE,
            "HTTP request with too long request line should not be parsed");
}

Test(http, should_parse_request_line_with_minimal_valid_path) {
  http_request_t request = {0};
  cr_assert(parse_http_request_line("GET / HTTP/1.1", &request) == PARSE_OK,
            "Request line with root path should be parsed");
  cr_assert_str_eq(request.path, "/", "Path should be /");
}

Test(http, should_parse_request_line_with_complex_valid_path) {
  http_request_t request = {0};
  cr_assert(parse_http_request_line("GET /path/to/resource.html HTTP/1.1", &request) == PARSE_OK,
            "Request line with complex path should be parsed");
  cr_assert_str_eq(request.path, "/path/to/resource.html", "Path should be /path/to/resource.html");
}

