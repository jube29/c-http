#include "../include/http.h"
#include <criterion/internal/test.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Test(http, should_parse_get) { cr_assert(parse_http_method("GET") == PARSE_OK, "GET method should be parsed"); }

Test(http, should_parse_post) { cr_assert(parse_http_method("POST") == PARSE_OK, "POST method should be parsed"); }

Test(http, should_parse_delete) { cr_assert(parse_http_method("HEAD") == PARSE_OK, "HEAD method should be parsed"); }

Test(http, should_not_parse_put) {
  cr_assert(parse_http_method("PUT") == PARSE_INVALID_METHOD, "PUT method should not be parsed");
}

Test(http, should_not_parse_patch) {
  cr_assert(parse_http_method("PATCH") == PARSE_INVALID_METHOD, "PATCH method should not be parsed");
}

Test(http, should_parse_protocol_http_1_0) {
  cr_assert(parse_http_protocol("HTTP/1.0") == PARSE_OK, "HTTP/1.0 protocol should be parsed");
}

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

Test(http, should_parse_valid_request_line) {
  http_request_t request = {0};
  cr_assert(parse_http_request_line("GET /index.html HTTP/1.0", &request) == PARSE_OK,
            "Valid request line should be parsed");
  cr_assert_str_eq(request.method, "GET", "Method should be GET");
  cr_assert_str_eq(request.path, "/index.html", "Path should be /index.html");
  cr_assert_str_eq(request.protocol, "HTTP/1.0", "Protocol should be HTTP/1.0");
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

Test(http, should_not_parse_request_line_with_invalid_path) {
  http_request_t request = {0};
  cr_assert(parse_http_request_line("GET index.html HTTP/1.0", &request) == PARSE_INVALID_PATH,
            "Request line with path missing leading slash should not be parsed");
  cr_assert(parse_http_request_line("GET \"\" HTTP/1.0", &request) == PARSE_INVALID_PATH,
            "Request line with empty path should not be parsed");
}

Test(http, should_not_parse_request_line_with_invalid_protocol) {
  http_request_t request = {0};
  cr_assert(parse_http_request_line("GET /index.html HTTP/1.1", &request) == PARSE_INVALID_PROTOCOL,
            "Request line with HTTP/1.1 protocol should not be parsed");
  cr_assert(parse_http_request_line("GET /index.html HTTP/2.0", &request) == PARSE_INVALID_PROTOCOL,
            "Request line with invalid protocol should not be parsed");
  cr_assert(parse_http_request_line("GET /index.html HTTPS/1.0", &request) == PARSE_INVALID_PROTOCOL,
            "Request line with invalid protocol should not be parsed");
}

Test(http, should_not_parse_http_request_without_crlf) {
  http_request_t request = {0};
  const char *http_data = "GET /test.html HTTP/1.0";
  cr_assert(parse_http_request(http_data, &request) == PARSE_UNTERMINATED_REQUEST_LINE,
            "HTTP request without CRLF should not be parsed");
}

Test(http, should_not_parse_http_request_with_too_long_request_line) {
  http_request_t request = {0};
  char long_line[4110];
  strcpy(long_line, "GET /");
  memset(long_line + 5, 'a', 4090);
  strcat(long_line, " HTTP/1.0\r\n");
  cr_assert(parse_http_request(long_line, &request) == PARSE_MALFORMED_REQUEST_LINE,
            "HTTP request with too long request line should not be parsed");
}

Test(http, should_parse_request_line_with_minimal_valid_path) {
  http_request_t request = {0};
  cr_assert(parse_http_request_line("GET / HTTP/1.0", &request) == PARSE_OK,
            "Request line with root path should be parsed");
  cr_assert_str_eq(request.path, "/", "Path should be /");
}

Test(http, should_parse_request_line_with_complex_valid_path) {
  http_request_t request = {0};
  cr_assert(parse_http_request_line("GET /path/to/resource.html HTTP/1.0", &request) == PARSE_OK,
            "Request line with complex path should be parsed");
  cr_assert_str_eq(request.path, "/path/to/resource.html", "Path should be /path/to/resource.html");
}

Test(http, should_parse_single_header) {
  http_request_t request = {0};
  const char *raw_headers = "Host: example.com\r\n";
  parse_http_headers(raw_headers, &request);

  cr_assert_eq(request.headers_count, 1, "Expected 1 header, got %d", (int)request.headers_count);
  cr_assert_str_eq(request.headers[0].key, "Host", "Header key should be 'Host'");
  cr_assert_str_eq(request.headers[0].value, "example.com", "Header value should be 'example.com'");

  free_http_headers(&request);
}

Test(http, should_parse_multiple_headers) {
  http_request_t request = {0};
  const char *raw_headers = "Host: example.com\r\n"
                            "User-Agent: TestAgent/1.0\r\n"
                            "Accept: text/html\r\n";
  parse_http_headers(raw_headers, &request);

  cr_assert_eq(request.headers_count, 3, "Should parse exactly 3 headers, got %d", (int)request.headers_count);

  cr_assert_str_eq(request.headers[0].key, "Host", "First header key should be 'Host'");
  cr_assert_str_eq(request.headers[0].value, "example.com", "First header value should be 'example.com'");

  cr_assert_str_eq(request.headers[1].key, "User-Agent", "Second header key should be 'User-Agent'");
  cr_assert_str_eq(request.headers[1].value, "TestAgent/1.0", "Second header value should be 'TestAgent/1.0'");

  cr_assert_str_eq(request.headers[2].key, "Accept", "Third header key should be 'Accept'");
  cr_assert_str_eq(request.headers[2].value, "text/html", "Third header value should be 'text/html'");

  free_http_headers(&request);
}

Test(http, should_parse_headers_with_spaces_around_colon) {
  http_request_t request = {0};
  const char *raw_headers = "Host : example.com\r\n"
                            "Content-Type:  text/plain  \r\n"
                            "Authorization:   Bearer token123   \r\n";
  parse_http_headers(raw_headers, &request);

  cr_assert_eq(request.headers_count, 3, "Should parse exactly 3 headers, got %d", (int)request.headers_count);

  cr_assert_str_eq(request.headers[0].key, "Host", "Expected key 'Host', got '%s'", request.headers[0].key);
  cr_assert_str_eq(request.headers[0].value, "example.com", "Expected value 'example.com', got '%s'",
                   request.headers[0].value);

  cr_assert_str_eq(request.headers[1].key, "Content-Type", "Expected key 'Content-Type', got '%s'",
                   request.headers[1].key);
  cr_assert_str_eq(request.headers[1].value, "text/plain", "Expected value 'text/plain', got '%s'",
                   request.headers[1].value);

  cr_assert_str_eq(request.headers[2].key, "Authorization", "Expected key 'Authorization', got '%s'",
                   request.headers[2].key);
  cr_assert_str_eq(request.headers[2].value, "Bearer token123", "Expected value 'Bearer token123', got '%s'",
                   request.headers[2].value);

  free_http_headers(&request);
}

Test(http, should_handle_headers_with_empty_values) {
  http_request_t request = {0};
  const char *raw_headers = "Host: example.com\r\n"
                            "Empty-Header:\r\n"
                            "Another-Header: \r\n";
  parse_http_headers(raw_headers, &request);

  cr_assert_eq(request.headers_count, 3, "Expected 3 headers, got %d", (int)request.headers_count);

  cr_assert_str_eq(request.headers[0].key, "Host", "Expected key 'Host', got '%s'", request.headers[0].key);
  cr_assert_str_eq(request.headers[0].value, "example.com", "Expected value 'example.com', got '%s'",
                   request.headers[0].value);

  cr_assert_str_eq(request.headers[1].key, "Empty-Header", "Expected key 'Empty-Header', got '%s'",
                   request.headers[1].key);
  cr_assert_str_eq(request.headers[1].value, "", "Expected empty value, got '%s'", request.headers[1].value);

  cr_assert_str_eq(request.headers[2].key, "Another-Header", "Expected key 'Another-Header', got '%s'",
                   request.headers[2].key);
  cr_assert_str_eq(request.headers[2].value, "", "Expected empty value, got '%s'", request.headers[2].value);

  free_http_headers(&request);
}

Test(http, should_handle_case_sensitive_header_keys) {
  http_request_t request = {0};
  const char *raw_headers = "host: example.com\r\n"
                            "HOST: EXAMPLE.COM\r\n"
                            "Host: Example.Com\r\n";
  parse_http_headers(raw_headers, &request);

  cr_assert_eq(request.headers_count, 3, "Should parse exactly 3 headers");

  cr_assert_str_eq(request.headers[0].key, "host", "First header key should preserve case");
  cr_assert_str_eq(request.headers[1].key, "HOST", "Second header key should preserve case");
  cr_assert_str_eq(request.headers[2].key, "Host", "Third header key should preserve case");

  free_http_headers(&request);
}

Test(http, should_handle_long_header_values) {
  http_request_t request = {0};
  char long_value[400];
  memset(long_value, 'a', 399);
  long_value[399] = '\0';

  char raw_headers[600];
  snprintf(raw_headers, sizeof(raw_headers),
           "Host: example.com\r\n"
           "Long-Header: %s\r\n",
           long_value);

  parse_http_headers(raw_headers, &request);

  cr_assert_eq(request.headers_count, 2, "Should parse exactly 2 headers");
  cr_assert_str_eq(request.headers[1].key, "Long-Header", "Header key should be 'Long-Header'");
  cr_assert_str_eq(request.headers[1].value, long_value, "Header value should match long value");

  free_http_headers(&request);
}

Test(http, should_reject_malformed_headers) {
  http_request_t request = {0};
  const char *raw_headers = "Host: example.com\r\n"
                            "MalformedHeaderWithoutColon\r\n"
                            "Another-Valid: header\r\n"
                            ": ValueWithoutKey\r\n";
  parse_http_headers(raw_headers, &request);

  cr_assert_eq(request.headers_count, 0, "Should parse no headers");

  free_http_headers(&request);
}

Test(http, should_reject_headers_with_continuation_lines) {
  http_request_t request = {0};
  const char *raw_headers = "Host: example.com\r\n"
                            "Multi-Line: first part\r\n"
                            " second part\r\n"
                            " third part\r\n"
                            "Regular: header\r\n";
  parse_http_headers(raw_headers, &request);

  cr_assert_eq(request.headers_count, 0, "Should parse no headers");

  free_http_headers(&request);
}

Test(http, should_handle_common_http_headers) {
  http_request_t request = {0};
  const char *raw_headers = "Host: www.example.com:8080\r\n"
                            "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
                            "AppleWebKit/537.36\r\n"
                            "Accept: "
                            "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
                            "Accept-Language: en-US,en;q=0.5\r\n"
                            "Accept-Encoding: gzip, deflate\r\n"
                            "Connection: keep-alive\r\n"
                            "Upgrade-Insecure-Requests: 1\r\n";
  parse_http_headers(raw_headers, &request);

  cr_assert_eq(request.headers_count, 7, "Should parse exactly 7 headers");

  cr_assert_str_eq(request.headers[0].key, "Host", "First header should be Host");
  cr_assert_str_eq(request.headers[0].value, "www.example.com:8080", "Host value should include port");

  cr_assert_str_eq(request.headers[5].key, "Connection", "Sixth header should be Connection");
  cr_assert_str_eq(request.headers[5].value, "keep-alive", "Connection value should be keep-alive");

  free_http_headers(&request);
}

Test(http, should_properly_initialize_headers_to_null) {
  http_request_t request = {0};
  cr_assert_null(request.headers, "Headers should be initialized to NULL");
  cr_assert_eq(request.headers_count, 0, "Headers count should be initialized to 0");
}

Test(http, should_free_headers_safely_when_null) {
  http_request_t request = {0};
  request.headers = NULL;
  request.headers_count = 0;

  free_http_headers(&request);

  cr_assert_null(request.headers, "Headers should remain NULL after free");
  cr_assert_eq(request.headers_count, 0, "Headers count should remain 0 after free");
}

Test(http, should_free_headers_and_reset_to_null) {
  http_request_t request = {0};
  const char *raw_headers = "Host: example.com\r\n"
                            "Content-Type: text/html\r\n";
  parse_result_e result = parse_http_headers(raw_headers, &request);

  cr_assert_neq(request.headers, NULL, "Headers should be set");
  cr_assert_eq(request.headers_count, 2, "Should have 2 headers");

  free_http_headers(&request);

  cr_assert_null(request.headers, "Headers should be NULL after free");
  cr_assert_eq(request.headers_count, 0, "Headers count should be 0 after free");
}

Test(http, should_handle_multiple_free_calls_safely) {
  http_request_t request = {0};
  const char *raw_headers = "Host: example.com\r\n";
  parse_http_headers(raw_headers, &request);

  free_http_headers(&request);
  cr_assert_null(request.headers, "Headers should be NULL after first free");

  free_http_headers(&request);
  cr_assert_null(request.headers, "Headers should remain NULL after second free");
}

Test(http, should_parse_complete_http_request_with_headers) {
  http_request_t request = {0};
  const char *http_data = "GET /api/users HTTP/1.0\r\n"
                          "Host: example.com\r\n"
                          "User-Agent: TestClient/1.0\r\n"
                          "Accept: text/plain\r\n"
                          "Authorization: Bearer token123\r\n\r\n";

  parse_result_e result = parse_http_request(http_data, &request);

  cr_assert_eq(result, PARSE_OK, "Expected PARSE_OK, got error code %d", result);

  cr_assert_str_eq(request.method, "GET", "Expected method 'GET', got '%s'", request.method);
  cr_assert_str_eq(request.path, "/api/users", "Expected path '/api/users', got '%s'", request.path);
  cr_assert_str_eq(request.protocol, "HTTP/1.0", "Expected protocol 'HTTP/1.0', got '%s'", request.protocol);

  cr_assert_eq(request.headers_count, 4, "Expected 4 headers, got %d", (int)request.headers_count);

  cr_assert_str_eq(request.headers[0].key, "Host", "Expected header[0] key 'Host', got '%s'", request.headers[0].key);
  cr_assert_str_eq(request.headers[0].value, "example.com", "Expected header[0] value 'example.com', got '%s'",
                   request.headers[0].value);

  cr_assert_str_eq(request.headers[1].key, "User-Agent", "Expected header[1] key 'User-Agent', got '%s'",
                   request.headers[1].key);
  cr_assert_str_eq(request.headers[1].value, "TestClient/1.0", "Expected header[1] value 'TestClient/1.0', got '%s'",
                   request.headers[1].value);

  cr_assert_str_eq(request.headers[2].key, "Accept", "Expected header[2] key 'Accept', got '%s'",
                   request.headers[2].key);
  cr_assert_str_eq(request.headers[2].value, "text/plain", "Expected header[2] value 'text/plain', got '%s'",
                   request.headers[2].value);

  cr_assert_str_eq(request.headers[3].key, "Authorization", "Expected header[3] key 'Authorization', got '%s'",
                   request.headers[3].key);
  cr_assert_str_eq(request.headers[3].value, "Bearer token123", "Expected header[3] value 'Bearer token123', got '%s'",
                   request.headers[3].value);

  free_http_headers(&request);
}

Test(http, should_parse_minimal_complete_http_request) {
  http_request_t request = {0};
  const char *http_data = "GET / HTTP/1.0\r\n"
                          "Host: localhost\r\n\r\n";

  parse_result_e result = parse_http_request(http_data, &request);

  cr_assert_eq(result, PARSE_OK, "Expected PARSE_OK, got error code %d", result);
  cr_assert_str_eq(request.method, "GET", "Expected method 'GET', got '%s'", request.method);
  cr_assert_str_eq(request.path, "/", "Expected path '/', got '%s'", request.path);
  cr_assert_str_eq(request.protocol, "HTTP/1.0", "Expected protocol 'HTTP/1.0', got '%s'", request.protocol);
  cr_assert_eq(request.headers_count, 1, "Expected 1 header, got %d", (int)request.headers_count);
  cr_assert_str_eq(request.headers[0].key, "Host", "Expected header key 'Host', got '%s'", request.headers[0].key);
  cr_assert_str_eq(request.headers[0].value, "localhost", "Expected header value 'localhost', got '%s'",
                   request.headers[0].value);

  free_http_headers(&request);
}

Test(http, should_parse_complete_http_request_without_headers) {
  http_request_t request = {0};
  const char *http_data = "GET /index.html HTTP/1.0\r\n\r\n";

  parse_result_e result = parse_http_request(http_data, &request);

  cr_assert_eq(result, PARSE_OK, "Expected PARSE_OK, got error code %d", result);

  cr_assert_str_eq(request.method, "GET", "Expected method 'GET', got '%s'", request.method);
  cr_assert_str_eq(request.path, "/index.html", "Expected path '/index.html', got '%s'", request.path);
  cr_assert_str_eq(request.protocol, "HTTP/1.0", "Expected protocol 'HTTP/1.0', got '%s'", request.protocol);

  cr_assert_eq(request.headers_count, 0, "Expected 0 headers, got %d", (int)request.headers_count);
  cr_assert_null(request.headers, "Expected headers to be NULL, got %p", request.headers);

  free_http_headers(&request);
}

Test(http, should_parse_post_request_with_simple_body) {
  http_request_t request = {0};
  const char *http_data = "POST /api/users HTTP/1.0\r\n"
                          "Host: example.com\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: 16\r\n\r\n"
                          "name=John&age=30";

  parse_result_e result = parse_http_request(http_data, &request);
  cr_assert_eq(result, PARSE_OK, "Expected PARSE_OK, got error code %d", result);

  cr_assert_str_eq(request.method, "POST", "Expected method 'POST', got '%s'", request.method);
  cr_assert_str_eq(request.path, "/api/users", "Expected path '/api/users', got '%s'", request.path);

  const char *body_start = strstr(http_data, "\r\n\r\n") + 4;
  size_t remaining_length = strlen(http_data) - (body_start - http_data);
  parse_result_e body_result = parse_http_body(body_start, remaining_length, &request);

  cr_assert_eq(body_result, PARSE_OK, "Expected PARSE_OK for body parsing, got error code %d", body_result);
  cr_assert_eq(request.body_length, 16, "Expected body length 16, got %zu", request.body_length);
  cr_assert_str_eq(request.body, "name=John&age=30", "Body content mismatch");

  free_http_request(&request);
}

Test(http, should_parse_request_with_empty_body) {
  http_request_t request = {0};
  const char *http_data = "POST /api/data HTTP/1.0\r\n"
                          "Host: example.com\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: 0\r\n\r\n";

  parse_result_e result = parse_http_request(http_data, &request);
  cr_assert_eq(result, PARSE_OK, "Expected PARSE_OK, got error code %d", result);

  const char *body_start = strstr(http_data, "\r\n\r\n") + 4;
  size_t remaining_length = strlen(http_data) - (body_start - http_data);
  parse_result_e body_result = parse_http_body(body_start, remaining_length, &request);

  cr_assert_eq(body_result, PARSE_OK, "Expected PARSE_OK for empty body, got error code %d", body_result);
  cr_assert_eq(request.body_length, 0, "Expected body length 0, got %zu", request.body_length);
  cr_assert_null(request.body, "Expected body to be NULL for empty body");

  free_http_request(&request);
}

Test(http, should_parse_request_with_text_body) {
  http_request_t request = {0};
  const char *http_data = "POST /api/note HTTP/1.0\r\n"
                          "Host: example.com\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: 12\r\n\r\n"
                          "Hello World!";

  parse_result_e result = parse_http_request(http_data, &request);
  cr_assert_eq(result, PARSE_OK, "Expected PARSE_OK, got error code %d", result);

  const char *body_start = strstr(http_data, "\r\n\r\n") + 4;
  size_t remaining_length = strlen(http_data) - (body_start - http_data);
  parse_result_e body_result = parse_http_body(body_start, remaining_length, &request);

  cr_assert_eq(body_result, PARSE_OK, "Expected PARSE_OK for text body, got error code %d", body_result);
  cr_assert_eq(request.body_length, 12, "Expected body length 12, got %zu", request.body_length);
  cr_assert_str_eq(request.body, "Hello World!", "Body content mismatch");

  free_http_request(&request);
}

Test(http, should_get_content_length_from_headers) {
  http_request_t request = {0};
  const char *raw_headers = "Host: example.com\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: 42\r\n";
  parse_http_headers(raw_headers, &request);

  const char *content_length_str = get_header_value(&request, "Content-Length");
  cr_assert_not_null(content_length_str, "Content-Length header should exist");
  size_t content_length = (size_t)atoi(content_length_str);
  cr_assert_eq(content_length, 42, "Expected content length 42, got %zu", content_length);

  free_http_headers(&request);
}

Test(http, should_get_header_value_by_key) {
  http_request_t request = {0};
  const char *raw_headers = "Host: example.com\r\n"
                            "Content-Type: text/plain\r\n"
                            "Authorization: Bearer token123\r\n";
  parse_http_headers(raw_headers, &request);

  const char *host = get_header_value(&request, "Host");
  const char *content_type = get_header_value(&request, "Content-Type");
  const char *auth = get_header_value(&request, "Authorization");
  const char *missing = get_header_value(&request, "Missing-Header");

  cr_assert_str_eq(host, "example.com", "Expected Host header value");
  cr_assert_str_eq(content_type, "text/plain", "Expected Content-Type header value");
  cr_assert_str_eq(auth, "Bearer token123", "Expected Authorization header value");
  cr_assert_null(missing, "Expected NULL for missing header");

  free_http_headers(&request);
}

Test(http, should_reject_body_larger_than_max_size) {
  http_request_t request = {0};
  size_t oversized_length = HTTP_MAX_BODY_SIZE + 1;
  char large_content_length[20];
  snprintf(large_content_length, sizeof(large_content_length), "%zu", oversized_length);

  const char *http_data_prefix = "POST /api HTTP/1.0\r\n"
                                 "Host: example.com\r\n"
                                 "Content-Type: text/plain\r\n"
                                 "Content-Length: ";
  const char *http_data_suffix = "\r\n\r\n";

  size_t request_size = strlen(http_data_prefix) + strlen(large_content_length) + strlen(http_data_suffix);
  char *http_data = malloc(request_size + oversized_length + 1);
  strcpy(http_data, http_data_prefix);
  strcat(http_data, large_content_length);
  strcat(http_data, http_data_suffix);

  memset(http_data + request_size, 'A', oversized_length);
  http_data[request_size + oversized_length] = '\0';

  parse_result_e result = parse_http_request(http_data, &request);
  cr_assert_eq(result, PARSE_BODY_TOO_LARGE, "Expected PARSE_BODY_TOO_LARGE from parse_http_request, got error code %d",
               result);

  free(http_data);
  free_http_request(&request);
}

Test(http, should_reject_invalid_content_length_header) {
  http_request_t request = {0};
  const char *http_data = "POST /api HTTP/1.0\r\n"
                          "Host: example.com\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: invalid\r\n\r\n"
                          "test body";

  parse_result_e result = parse_http_request(http_data, &request);
  cr_assert_eq(result, PARSE_CONTENT_LENGTH_INVALID,
               "Expected PARSE_CONTENT_LENGTH_INVALID from parse_http_request, got error code %d", result);

  free_http_request(&request);
}

Test(http, should_reject_negative_content_length) {
  http_request_t request = {0};
  const char *http_data = "POST /api HTTP/1.0\r\n"
                          "Host: example.com\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: -5\r\n\r\n"
                          "test";

  parse_result_e result = parse_http_request(http_data, &request);
  cr_assert_eq(result, PARSE_CONTENT_LENGTH_INVALID,
               "Expected PARSE_CONTENT_LENGTH_INVALID from parse_http_request, got error code %d", result);

  free_http_request(&request);
}

Test(http, should_reject_content_length_mismatch) {
  http_request_t request = {0};
  const char *http_data = "POST /api HTTP/1.0\r\n"
                          "Host: example.com\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: 50\r\n\r\n"
                          "Short body";

  parse_result_e result = parse_http_request(http_data, &request);
  cr_assert_eq(result, PARSE_CONTENT_LENGTH_MISMATCH,
               "Expected PARSE_CONTENT_LENGTH_MISMATCH from parse_http_request, got error code %d", result);

  free_http_request(&request);
}

Test(http, should_handle_incomplete_body_data) {
  http_request_t request = {0};
  const char *http_data = "POST /api HTTP/1.0\r\n"
                          "Host: example.com\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: 20\r\n\r\n"
                          "Incomplete";

  parse_result_e result = parse_http_request(http_data, &request);
  cr_assert_eq(result, PARSE_CONTENT_LENGTH_MISMATCH,
               "Expected PARSE_CONTENT_LENGTH_MISMATCH from parse_http_request, got error code %d", result);

  free_http_request(&request);
}

Test(http, should_reject_multiple_content_length_headers) {
  http_request_t request = {0};
  const char *http_data = "POST /api HTTP/1.0\r\n"
                          "Host: example.com\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: 10\r\n"
                          "Content-Length: 20\r\n\r\n"
                          "test body1";

  parse_result_e result = parse_http_request(http_data, &request);
  cr_assert_eq(result, PARSE_OK, "Expected PARSE_OK from parse_http_request, got error code %d", result);

  free_http_request(&request);
}

Test(http, should_handle_body_with_null_bytes) {
  http_request_t request = {0};
  const char *http_data = "POST /api HTTP/1.0\r\n"
                          "Host: example.com\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: 11\r\n\r\n";

  char *full_data = malloc(strlen(http_data) + 11 + 1);
  strcpy(full_data, http_data);
  char *body_pos = full_data + strlen(http_data);
  memcpy(body_pos, "Hello\0World", 11);

  parse_result_e result = parse_http_request(full_data, &request);
  cr_assert_neq(result, PARSE_OK, "Request parsing should fail due to null terminator in body");

  const char *body_start = strstr(full_data, "\r\n\r\n") + 4;
  parse_result_e body_result = parse_http_body(body_start, 11, &request);
  cr_assert_eq(body_result, PARSE_OK, "Expected PARSE_OK for body with null bytes, got %d", body_result);
  cr_assert_eq(request.body_length, 11, "Expected body length 11, got %zu", request.body_length);

  cr_assert_eq(request.body[5], '\0', "Expected null byte at position 5");
  cr_assert_eq(request.body[6], 'W', "Expected 'W' at position 6");

  free(full_data);
  free_http_request(&request);
}

Test(http, should_handle_zero_content_length_with_no_body) {
  http_request_t request = {0};
  const char *http_data = "POST /api HTTP/1.0\r\n"
                          "Host: example.com\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: 0\r\n\r\n";

  parse_result_e result = parse_http_request(http_data, &request);
  cr_assert_eq(result, PARSE_OK, "Request parsing should succeed");

  const char *body_start = strstr(http_data, "\r\n\r\n") + 4;
  size_t remaining_length = strlen(http_data) - (body_start - http_data);
  parse_result_e body_result = parse_http_body(body_start, remaining_length, &request);
  cr_assert_eq(body_result, PARSE_OK, "Expected PARSE_OK for zero content length with no body, got %d", body_result);
  cr_assert_eq(request.body_length, 0, "Expected body length 0, got %zu", request.body_length);
  cr_assert_null(request.body, "Expected body to be NULL for zero content length");

  free_http_request(&request);
}

Test(http, should_handle_whitespace_only_body) {
  http_request_t request = {0};
  const char *whitespace_body = "   \t\n\r\n  ";
  char content_length_str[20];
  snprintf(content_length_str, sizeof(content_length_str), "%zu", strlen(whitespace_body));

  char *http_data = malloc(200 + strlen(whitespace_body));
  strcpy(http_data, "POST /api HTTP/1.0\r\n"
                    "Host: example.com\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: ");
  strcat(http_data, content_length_str);
  strcat(http_data, "\r\n\r\n");
  strcat(http_data, whitespace_body);

  parse_result_e result = parse_http_request(http_data, &request);
  cr_assert_eq(result, PARSE_OK, "Request parsing should succeed");

  const char *body_start = strstr(http_data, "\r\n\r\n") + 4;
  size_t remaining_length = strlen(http_data) - (body_start - http_data);
  parse_result_e body_result = parse_http_body(body_start, remaining_length, &request);
  cr_assert_eq(body_result, PARSE_OK, "Expected PARSE_OK for whitespace body, got %d", body_result);
  cr_assert_eq(request.body_length, strlen(whitespace_body), "Expected body length %zu, got %zu",
               strlen(whitespace_body), request.body_length);

  free(http_data);
  free_http_request(&request);
}

Test(http, should_accept_text_plain_content_type) {
  http_request_t request = {0};
  const char *http_data = "POST /api HTTP/1.0\r\n"
                          "Host: example.com\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: 5\r\n\r\n"
                          "hello";

  parse_result_e result = parse_http_request(http_data, &request);
  cr_assert_eq(result, PARSE_OK, "Expected PARSE_OK for text/plain content type, got error code %d", result);

  free_http_request(&request);
}

Test(http, should_reject_application_json_content_type) {
  http_request_t request = {0};
  const char *http_data = "POST /api HTTP/1.0\r\n"
                          "Host: example.com\r\n"
                          "Content-Type: application/json\r\n"
                          "Content-Length: 13\r\n\r\n"
                          "{\"key\":\"val\"}";

  parse_result_e result = parse_http_request(http_data, &request);
  cr_assert_eq(result, PARSE_UNSUPPORTED_CONTENT_TYPE,
               "Expected PARSE_UNSUPPORTED_CONTENT_TYPE for application/json, got error code %d", result);

  free_http_request(&request);
}

Test(http, should_reject_text_html_content_type) {
  http_request_t request = {0};
  const char *http_data = "POST /api HTTP/1.0\r\n"
                          "Host: example.com\r\n"
                          "Content-Type: text/html\r\n"
                          "Content-Length: 13\r\n\r\n"
                          "<html></html>";

  parse_result_e result = parse_http_request(http_data, &request);
  cr_assert_eq(result, PARSE_UNSUPPORTED_CONTENT_TYPE,
               "Expected PARSE_UNSUPPORTED_CONTENT_TYPE for text/html, got error code %d", result);

  free_http_request(&request);
}

Test(http, should_reject_post_request_without_content_type_header) {
  http_request_t request = {0};
  const char *http_data = "POST /api HTTP/1.0\r\n"
                          "Host: example.com\r\n"
                          "Content-Length: 5\r\n\r\n"
                          "hello";

  parse_result_e result = parse_http_request(http_data, &request);
  cr_assert_eq(result, PARSE_UNSUPPORTED_CONTENT_TYPE,
               "Expected PARSE_UNSUPPORTED_CONTENT_TYPE for POST without Content-Type header, got error code %d",
               result);

  free_http_request(&request);
}

Test(http, should_accept_get_request_without_content_type_header) {
  http_request_t request = {0};
  const char *http_data = "GET /api HTTP/1.0\r\n"
                          "Host: example.com\r\n\r\n";

  parse_result_e result = parse_http_request(http_data, &request);
  cr_assert_eq(result, PARSE_OK, "Expected PARSE_OK for GET without Content-Type header, got error code %d", result);

  free_http_request(&request);
}

Test(http, should_build_status_line_for_parse_ok) {
  http_response_t response = {0};
  build_status_line(PARSE_OK, &response);

  cr_assert_str_eq(response.protocol, "HTTP/1.0", "Expected protocol 'HTTP/1.0', got '%s'", response.protocol);
  cr_assert_eq(response.status_code, 200, "Expected status code 200, got %u", response.status_code);
  cr_assert_str_eq(response.reason_phrase, "OK", "Expected reason phrase 'OK', got '%s'", response.reason_phrase);
}

Test(http, should_build_status_line_for_parse_invalid_method) {
  http_response_t response = {0};
  build_status_line(PARSE_INVALID_METHOD, &response);

  cr_assert_str_eq(response.protocol, "HTTP/1.0", "Expected protocol 'HTTP/1.0', got '%s'", response.protocol);
  cr_assert_eq(response.status_code, 405, "Expected status code 405, got %u", response.status_code);
  cr_assert_str_eq(response.reason_phrase, "Method Not Allowed",
                   "Expected reason phrase 'Method Not Allowed', got '%s'", response.reason_phrase);
}

Test(http, should_build_status_line_for_parse_invalid_path) {
  http_response_t response = {0};
  build_status_line(PARSE_INVALID_PATH, &response);

  cr_assert_str_eq(response.protocol, "HTTP/1.0", "Expected protocol 'HTTP/1.0', got '%s'", response.protocol);
  cr_assert_eq(response.status_code, 400, "Expected status code 400, got %u", response.status_code);
  cr_assert_str_eq(response.reason_phrase, "Bad Request", "Expected reason phrase 'Bad Request', got '%s'",
                   response.reason_phrase);
}

Test(http, should_build_status_line_for_parse_invalid_protocol) {
  http_response_t response = {0};
  build_status_line(PARSE_INVALID_PROTOCOL, &response);

  cr_assert_str_eq(response.protocol, "HTTP/1.0", "Expected protocol 'HTTP/1.0', got '%s'", response.protocol);
  cr_assert_eq(response.status_code, 505, "Expected status code 505, got %u", response.status_code);
  cr_assert_str_eq(response.reason_phrase, "HTTP Version Not Supported",
                   "Expected reason phrase 'HTTP Version Not Supported', got '%s'", response.reason_phrase);
}

Test(http, should_build_status_line_for_parse_malformed_request_line) {
  http_response_t response = {0};
  build_status_line(PARSE_MALFORMED_REQUEST_LINE, &response);

  cr_assert_str_eq(response.protocol, "HTTP/1.0", "Expected protocol 'HTTP/1.0', got '%s'", response.protocol);
  cr_assert_eq(response.status_code, 400, "Expected status code 400, got %u", response.status_code);
  cr_assert_str_eq(response.reason_phrase, "Bad Request", "Expected reason phrase 'Bad Request', got '%s'",
                   response.reason_phrase);
}

Test(http, should_build_status_line_for_parse_too_many_headers) {
  http_response_t response = {0};
  build_status_line(PARSE_TOO_MANY_HEADERS, &response);

  cr_assert_str_eq(response.protocol, "HTTP/1.0", "Expected protocol 'HTTP/1.0', got '%s'", response.protocol);
  cr_assert_eq(response.status_code, 413, "Expected status code 413, got %u", response.status_code);
  cr_assert_str_eq(response.reason_phrase, "Payload Too Large", "Expected reason phrase 'Payload Too Large', got '%s'",
                   response.reason_phrase);
}

Test(http, should_build_status_line_for_parse_body_too_large) {
  http_response_t response = {0};
  build_status_line(PARSE_BODY_TOO_LARGE, &response);

  cr_assert_str_eq(response.protocol, "HTTP/1.0", "Expected protocol 'HTTP/1.0', got '%s'", response.protocol);
  cr_assert_eq(response.status_code, 413, "Expected status code 413, got %u", response.status_code);
  cr_assert_str_eq(response.reason_phrase, "Payload Too Large", "Expected reason phrase 'Payload Too Large', got '%s'",
                   response.reason_phrase);
}

Test(http, should_build_status_line_for_parse_unsupported_content_type) {
  http_response_t response = {0};
  build_status_line(PARSE_UNSUPPORTED_CONTENT_TYPE, &response);

  cr_assert_str_eq(response.protocol, "HTTP/1.0", "Expected protocol 'HTTP/1.0', got '%s'", response.protocol);
  cr_assert_eq(response.status_code, 415, "Expected status code 415, got %u", response.status_code);
  cr_assert_str_eq(response.reason_phrase, "Unsupported Media Type",
                   "Expected reason phrase 'Unsupported Media Type', got '%s'", response.reason_phrase);
}

Test(http, should_build_status_line_for_parse_memory_error) {
  http_response_t response = {0};
  build_status_line(PARSE_MEMORY_ERROR, &response);

  cr_assert_str_eq(response.protocol, "HTTP/1.0", "Expected protocol 'HTTP/1.0', got '%s'", response.protocol);
  cr_assert_eq(response.status_code, 500, "Expected status code 500, got %u", response.status_code);
  cr_assert_str_eq(response.reason_phrase, "Internal Server Error",
                   "Expected reason phrase 'Internal Server Error', got '%s'", response.reason_phrase);
}

Test(http, should_build_response_headers_without_body) {
  http_response_t response = {0};
  response.body = NULL;
  response.body_length = 0;

  build_response_headers(&response);

  cr_assert_eq(response.headers_count, 3, "Expected 3 headers, got %zu", response.headers_count);
  cr_assert_not_null(response.headers, "Headers should not be NULL");

  cr_assert_str_eq(response.headers[0].key, "Connection", "Expected Connection header key, got '%s'",
                   response.headers[0].key);
  cr_assert_str_eq(response.headers[0].value, "close", "Expected Connection header value 'close', got '%s'",
                   response.headers[0].value);

  cr_assert_str_eq(response.headers[1].key, "Content-Length", "Expected Content-Length header key, got '%s'",
                   response.headers[1].key);
  cr_assert_str_eq(response.headers[1].value, "0", "Expected Content-Length header value '0', got '%s'",
                   response.headers[1].value);

  cr_assert_str_eq(response.headers[2].key, "Content-Type", "Expected Content-Type header key, got '%s'",
                   response.headers[2].key);
  cr_assert_str_eq(response.headers[2].value, "text/plain", "Expected Content-Type header value 'text/plain', got '%s'",
                   response.headers[2].value);

  free_http_response(&response);
}

Test(http, should_build_response_headers_with_body) {
  http_response_t response = {0};
  const char *body_content = "Hello World!";
  response.body = malloc(strlen(body_content) + 1);
  strcpy(response.body, body_content);
  response.body_length = strlen(body_content);

  build_response_headers(&response);

  cr_assert_eq(response.headers_count, 3, "Expected 3 headers, got %zu", response.headers_count);
  cr_assert_not_null(response.headers, "Headers should not be NULL");

  cr_assert_str_eq(response.headers[0].key, "Connection", "Expected Connection header key, got '%s'",
                   response.headers[0].key);
  cr_assert_str_eq(response.headers[0].value, "close", "Expected Connection header value 'close', got '%s'",
                   response.headers[0].value);

  cr_assert_str_eq(response.headers[1].key, "Content-Length", "Expected Content-Length header key, got '%s'",
                   response.headers[1].key);
  cr_assert_str_eq(response.headers[1].value, "12", "Expected Content-Length header value '12', got '%s'",
                   response.headers[1].value);

  cr_assert_str_eq(response.headers[2].key, "Content-Type", "Expected Content-Type header key, got '%s'",
                   response.headers[2].key);
  cr_assert_str_eq(response.headers[2].value, "text/plain", "Expected Content-Type header value 'text/plain', got '%s'",
                   response.headers[2].value);

  free_http_response(&response);
}

Test(http, should_build_response_headers_with_empty_body) {
  http_response_t response = {0};
  response.body = malloc(1);
  response.body[0] = '\0';
  response.body_length = 0;

  build_response_headers(&response);

  cr_assert_eq(response.headers_count, 3, "Expected 3 headers for empty body, got %zu", response.headers_count);
  cr_assert_not_null(response.headers, "Headers should not be NULL");

  cr_assert_str_eq(response.headers[0].key, "Connection", "Expected Connection header key, got '%s'",
                   response.headers[0].key);
  cr_assert_str_eq(response.headers[0].value, "close", "Expected Connection header value 'close', got '%s'",
                   response.headers[0].value);

  cr_assert_str_eq(response.headers[1].key, "Content-Length", "Expected Content-Length header key, got '%s'",
                   response.headers[1].key);
  cr_assert_str_eq(response.headers[1].value, "0", "Expected Content-Length header value '0', got '%s'",
                   response.headers[1].value);

  cr_assert_str_eq(response.headers[2].key, "Content-Type", "Expected Content-Type header key, got '%s'",
                   response.headers[2].key);
  cr_assert_str_eq(response.headers[2].value, "text/plain", "Expected Content-Type header value 'text/plain', got '%s'",
                   response.headers[2].value);

  free_http_response(&response);
}

Test(http, should_handle_null_response_in_build_response_headers) { build_response_headers(NULL); }

Test(http, should_build_response_headers_with_large_body) {
  http_response_t response = {0};
  size_t large_size = 1000;
  response.body = malloc(large_size + 1);
  memset(response.body, 'A', large_size);
  response.body[large_size] = '\0';
  response.body_length = large_size;

  build_response_headers(&response);

  cr_assert_eq(response.headers_count, 3, "Expected 3 headers, got %zu", response.headers_count);

  cr_assert_str_eq(response.headers[1].key, "Content-Length", "Expected Content-Length header key, got '%s'",
                   response.headers[1].key);
  cr_assert_str_eq(response.headers[1].value, "1000", "Expected Content-Length header value '1000', got '%s'",
                   response.headers[1].value);

  free_http_response(&response);
}

Test(http, should_set_response_body_with_valid_string) {
  http_response_t response = {0};
  const char *body_text = "Hello, World!";

  parse_result_e result = set_response_body(&response, body_text);

  cr_assert_eq(result, PARSE_OK, "Expected PARSE_OK, got error code %d", result);
  cr_assert_not_null(response.body, "Response body should not be NULL");
  cr_assert_str_eq(response.body, body_text, "Response body should match input");
  cr_assert_eq(response.body_length, strlen(body_text), "Body length should match string length");

  free_http_response(&response);
}

Test(http, should_set_response_body_with_empty_string) {
  http_response_t response = {0};
  const char *body_text = "";

  parse_result_e result = set_response_body(&response, body_text);

  cr_assert_eq(result, PARSE_OK, "Expected PARSE_OK, got error code %d", result);
  cr_assert_not_null(response.body, "Response body should not be NULL even for empty string");
  cr_assert_str_eq(response.body, "", "Response body should be empty string");
  cr_assert_eq(response.body_length, 0, "Body length should be 0 for empty string");

  free_http_response(&response);
}

Test(http, should_set_response_body_with_null_body) {
  http_response_t response = {0};

  parse_result_e result = set_response_body(&response, NULL);

  cr_assert_eq(result, PARSE_OK, "Expected PARSE_OK, got error code %d", result);
  cr_assert_null(response.body, "Response body should be NULL");
  cr_assert_eq(response.body_length, 0, "Body length should be 0 for NULL body");
}

Test(http, should_replace_existing_response_body) {
  http_response_t response = {0};
  const char *first_body = "First body";
  const char *second_body = "Second body text";

  parse_result_e result1 = set_response_body(&response, first_body);
  cr_assert_eq(result1, PARSE_OK, "First set should succeed");
  cr_assert_str_eq(response.body, first_body, "Should have first body");

  parse_result_e result2 = set_response_body(&response, second_body);
  cr_assert_eq(result2, PARSE_OK, "Second set should succeed");
  cr_assert_str_eq(response.body, second_body, "Should have second body");
  cr_assert_eq(response.body_length, strlen(second_body), "Body length should match second body");

  free_http_response(&response);
}

Test(http, should_reject_null_response_in_set_response_body) {
  const char *body_text = "Test body";

  parse_result_e result = set_response_body(NULL, body_text);

  cr_assert_eq(result, PARSE_MEMORY_ERROR, "Expected PARSE_MEMORY_ERROR for NULL response");
}

Test(http, should_reject_oversized_response_body) {
  http_response_t response = {0};
  size_t oversized_length = HTTP_MAX_BODY_SIZE + 1;
  char *large_body = malloc(oversized_length + 1);
  memset(large_body, 'A', oversized_length);
  large_body[oversized_length] = '\0';

  parse_result_e result = set_response_body(&response, large_body);

  cr_assert_eq(result, PARSE_BODY_TOO_LARGE, "Expected PARSE_BODY_TOO_LARGE for oversized body");
  cr_assert_null(response.body, "Response body should remain NULL after failed set");
  cr_assert_eq(response.body_length, 0, "Body length should remain 0 after failed set");

  free(large_body);
}

Test(http, should_clear_body_when_setting_null_after_existing_body) {
  http_response_t response = {0};
  const char *initial_body = "Initial body content";

  parse_result_e result1 = set_response_body(&response, initial_body);
  cr_assert_eq(result1, PARSE_OK, "Initial set should succeed");
  cr_assert_not_null(response.body, "Should have initial body");

  parse_result_e result2 = set_response_body(&response, NULL);
  cr_assert_eq(result2, PARSE_OK, "Setting NULL should succeed");
  cr_assert_null(response.body, "Body should be NULL after setting to NULL");
  cr_assert_eq(response.body_length, 0, "Body length should be 0 after setting to NULL");
}

Test(http, should_build_complete_response_for_parse_ok) {
  http_response_t response = {0};
  const char *body_content = "Request processed successfully";

  parse_result_e result = build_response(PARSE_OK, body_content, &response);

  cr_assert_eq(result, PARSE_OK, "build_response should succeed");

  cr_assert_str_eq(response.protocol, "HTTP/1.0", "Protocol should be HTTP/1.0");
  cr_assert_eq(response.status_code, 200, "Status code should be 200");
  cr_assert_str_eq(response.reason_phrase, "OK", "Reason phrase should be OK");

  cr_assert_not_null(response.body, "Response body should not be NULL");
  cr_assert_str_eq(response.body, body_content, "Response body should match input");
  cr_assert_eq(response.body_length, strlen(body_content), "Body length should match");

  cr_assert_eq(response.headers_count, 3, "Should have 3 headers");
  cr_assert_str_eq(response.headers[0].key, "Connection", "First header should be Connection");
  cr_assert_str_eq(response.headers[0].value, "close", "Connection should be close");
  cr_assert_str_eq(response.headers[1].key, "Content-Length", "Second header should be Content-Length");
  cr_assert_str_eq(response.headers[1].value, "30", "Content-Length should match body length");
  cr_assert_str_eq(response.headers[2].key, "Content-Type", "Third header should be Content-Type");
  cr_assert_str_eq(response.headers[2].value, "text/plain", "Content-Type should be text/plain");

  free_http_response(&response);
}

Test(http, should_build_complete_response_for_parse_invalid_method) {
  http_response_t response = {0};
  const char *body_content = "Method not allowed";

  parse_result_e result = build_response(PARSE_INVALID_METHOD, body_content, &response);

  cr_assert_eq(result, PARSE_OK, "build_response should succeed");

  cr_assert_str_eq(response.protocol, "HTTP/1.0", "Protocol should be HTTP/1.0");
  cr_assert_eq(response.status_code, 405, "Status code should be 405");
  cr_assert_str_eq(response.reason_phrase, "Method Not Allowed", "Reason phrase should be Method Not Allowed");

  cr_assert_not_null(response.body, "Response body should not be NULL");
  cr_assert_str_eq(response.body, body_content, "Response body should match input");
  cr_assert_eq(response.body_length, strlen(body_content), "Body length should match");

  cr_assert_eq(response.headers_count, 3, "Should have 3 headers");
  cr_assert_str_eq(response.headers[1].key, "Content-Length", "Second header should be Content-Length");
  cr_assert_str_eq(response.headers[1].value, "18", "Content-Length should match body length");

  free_http_response(&response);
}

Test(http, should_build_complete_response_for_parse_invalid_path) {
  http_response_t response = {0};
  const char *body_content = "Bad request - invalid path";

  parse_result_e result = build_response(PARSE_INVALID_PATH, body_content, &response);

  cr_assert_eq(result, PARSE_OK, "build_response should succeed");

  cr_assert_str_eq(response.protocol, "HTTP/1.0", "Protocol should be HTTP/1.0");
  cr_assert_eq(response.status_code, 400, "Status code should be 400");
  cr_assert_str_eq(response.reason_phrase, "Bad Request", "Reason phrase should be Bad Request");

  cr_assert_not_null(response.body, "Response body should not be NULL");
  cr_assert_str_eq(response.body, body_content, "Response body should match input");
  cr_assert_eq(response.body_length, strlen(body_content), "Body length should match");

  free_http_response(&response);
}

Test(http, should_build_complete_response_for_parse_invalid_protocol) {
  http_response_t response = {0};
  const char *body_content = "HTTP version not supported";

  parse_result_e result = build_response(PARSE_INVALID_PROTOCOL, body_content, &response);

  cr_assert_eq(result, PARSE_OK, "build_response should succeed");

  cr_assert_str_eq(response.protocol, "HTTP/1.0", "Protocol should be HTTP/1.0");
  cr_assert_eq(response.status_code, 505, "Status code should be 505");
  cr_assert_str_eq(response.reason_phrase, "HTTP Version Not Supported", "Reason phrase should match");

  cr_assert_not_null(response.body, "Response body should not be NULL");
  cr_assert_str_eq(response.body, body_content, "Response body should match input");

  free_http_response(&response);
}

Test(http, should_build_complete_response_for_parse_body_too_large) {
  http_response_t response = {0};
  const char *body_content = "Request entity too large";

  parse_result_e result = build_response(PARSE_BODY_TOO_LARGE, body_content, &response);

  cr_assert_eq(result, PARSE_OK, "build_response should succeed");

  cr_assert_str_eq(response.protocol, "HTTP/1.0", "Protocol should be HTTP/1.0");
  cr_assert_eq(response.status_code, 413, "Status code should be 413");
  cr_assert_str_eq(response.reason_phrase, "Payload Too Large", "Reason phrase should be Payload Too Large");

  cr_assert_not_null(response.body, "Response body should not be NULL");
  cr_assert_str_eq(response.body, body_content, "Response body should match input");

  free_http_response(&response);
}

Test(http, should_build_complete_response_for_parse_unsupported_content_type) {
  http_response_t response = {0};
  const char *body_content = "Unsupported media type";

  parse_result_e result = build_response(PARSE_UNSUPPORTED_CONTENT_TYPE, body_content, &response);

  cr_assert_eq(result, PARSE_OK, "build_response should succeed");

  cr_assert_str_eq(response.protocol, "HTTP/1.0", "Protocol should be HTTP/1.0");
  cr_assert_eq(response.status_code, 415, "Status code should be 415");
  cr_assert_str_eq(response.reason_phrase, "Unsupported Media Type", "Reason phrase should match");

  cr_assert_not_null(response.body, "Response body should not be NULL");
  cr_assert_str_eq(response.body, body_content, "Response body should match input");

  free_http_response(&response);
}

Test(http, should_build_complete_response_for_parse_memory_error) {
  http_response_t response = {0};
  const char *body_content = "Internal server error";

  parse_result_e result = build_response(PARSE_MEMORY_ERROR, body_content, &response);

  cr_assert_eq(result, PARSE_OK, "build_response should succeed");

  cr_assert_str_eq(response.protocol, "HTTP/1.0", "Protocol should be HTTP/1.0");
  cr_assert_eq(response.status_code, 500, "Status code should be 500");
  cr_assert_str_eq(response.reason_phrase, "Internal Server Error", "Reason phrase should match");

  cr_assert_not_null(response.body, "Response body should not be NULL");
  cr_assert_str_eq(response.body, body_content, "Response body should match input");

  free_http_response(&response);
}

Test(http, should_build_complete_response_with_empty_body) {
  http_response_t response = {0};
  const char *body_content = "";

  parse_result_e result = build_response(PARSE_OK, body_content, &response);

  cr_assert_eq(result, PARSE_OK, "build_response should succeed");

  cr_assert_str_eq(response.protocol, "HTTP/1.0", "Protocol should be HTTP/1.0");
  cr_assert_eq(response.status_code, 200, "Status code should be 200");
  cr_assert_str_eq(response.reason_phrase, "OK", "Reason phrase should be OK");

  cr_assert_not_null(response.body, "Response body should not be NULL even for empty string");
  cr_assert_str_eq(response.body, "", "Response body should be empty string");
  cr_assert_eq(response.body_length, 0, "Body length should be 0");

  cr_assert_eq(response.headers_count, 3, "Should have 3 headers");
  cr_assert_str_eq(response.headers[1].value, "0", "Content-Length should be 0");

  free_http_response(&response);
}

Test(http, should_build_complete_response_with_null_body) {
  http_response_t response = {0};

  parse_result_e result = build_response(PARSE_OK, NULL, &response);

  cr_assert_eq(result, PARSE_OK, "build_response should succeed");

  cr_assert_str_eq(response.protocol, "HTTP/1.0", "Protocol should be HTTP/1.0");
  cr_assert_eq(response.status_code, 200, "Status code should be 200");
  cr_assert_str_eq(response.reason_phrase, "OK", "Reason phrase should be OK");

  cr_assert_null(response.body, "Response body should be NULL");
  cr_assert_eq(response.body_length, 0, "Body length should be 0");

  cr_assert_eq(response.headers_count, 3, "Should have 3 headers");
  cr_assert_str_eq(response.headers[1].value, "0", "Content-Length should be 0");

  free_http_response(&response);
}

Test(http, should_build_complete_response_with_large_body) {
  http_response_t response = {0};
  size_t large_size = 1000;
  char *large_body = malloc(large_size + 1);
  memset(large_body, 'X', large_size);
  large_body[large_size] = '\0';

  parse_result_e result = build_response(PARSE_OK, large_body, &response);

  cr_assert_eq(result, PARSE_OK, "build_response should succeed");

  cr_assert_str_eq(response.protocol, "HTTP/1.0", "Protocol should be HTTP/1.0");
  cr_assert_eq(response.status_code, 200, "Status code should be 200");

  cr_assert_not_null(response.body, "Response body should not be NULL");
  cr_assert_str_eq(response.body, large_body, "Response body should match input");
  cr_assert_eq(response.body_length, large_size, "Body length should match");

  cr_assert_eq(response.headers_count, 3, "Should have 3 headers");
  cr_assert_str_eq(response.headers[1].value, "1000", "Content-Length should be 1000");

  free(large_body);
  free_http_response(&response);
}

Test(http, should_reject_build_response_with_null_response_struct) {
  const char *body_content = "Test body";

  parse_result_e result = build_response(PARSE_OK, body_content, NULL);

  cr_assert_eq(result, PARSE_MEMORY_ERROR, "build_response should reject NULL response struct");
}

Test(http, should_reject_build_response_with_oversized_body) {
  http_response_t response = {0};
  size_t oversized_length = HTTP_MAX_BODY_SIZE + 1;
  char *oversized_body = malloc(oversized_length + 1);
  memset(oversized_body, 'A', oversized_length);
  oversized_body[oversized_length] = '\0';

  parse_result_e result = build_response(PARSE_OK, oversized_body, &response);

  cr_assert_eq(result, PARSE_BODY_TOO_LARGE, "build_response should reject oversized body");

  cr_assert_null(response.body, "Response body should remain NULL after failed build");
  cr_assert_eq(response.body_length, 0, "Body length should remain 0 after failed build");

  free(oversized_body);
  free_http_response(&response);
}

Test(http, should_build_response_with_mixed_parse_results_and_body_content) {
  http_response_t response1 = {0};
  http_response_t response2 = {0};
  http_response_t response3 = {0};

  const char *success_body = "Success message";
  const char *error_body = "Error occurred";
  const char *warning_body = "Warning: something happened";

  parse_result_e result1 = build_response(PARSE_OK, success_body, &response1);
  parse_result_e result2 = build_response(PARSE_INVALID_METHOD, error_body, &response2);
  parse_result_e result3 = build_response(PARSE_CONTENT_LENGTH_INVALID, warning_body, &response3);

  cr_assert_eq(result1, PARSE_OK, "First build_response should succeed");
  cr_assert_eq(result2, PARSE_OK, "Second build_response should succeed");
  cr_assert_eq(result3, PARSE_OK, "Third build_response should succeed");

  cr_assert_eq(response1.status_code, 200, "First response should have status 200");
  cr_assert_eq(response2.status_code, 405, "Second response should have status 405");
  cr_assert_eq(response3.status_code, 400, "Third response should have status 400");

  cr_assert_str_eq(response1.body, success_body, "First response body should match");
  cr_assert_str_eq(response2.body, error_body, "Second response body should match");
  cr_assert_str_eq(response3.body, warning_body, "Third response body should match");

  free_http_response(&response1);
  free_http_response(&response2);
  free_http_response(&response3);
}

Test(http, should_build_response_with_all_standard_headers_correctly_formatted) {
  http_response_t response = {0};
  const char *body_content = "Response with all headers";

  parse_result_e result = build_response(PARSE_OK, body_content, &response);

  cr_assert_eq(result, PARSE_OK, "build_response should succeed");
  cr_assert_eq(response.headers_count, 3, "Should have exactly 3 standard headers");

  cr_assert_str_eq(response.headers[0].key, "Connection", "First header key should be Connection");
  cr_assert_str_eq(response.headers[0].value, "close", "Connection value should be close");
  cr_assert(strlen(response.headers[0].key) < HTTP_HEADER_KEY_LEN, "Connection key should fit in buffer");
  cr_assert(strlen(response.headers[0].value) < HTTP_HEADER_VALUE_LEN, "Connection value should fit in buffer");

  cr_assert_str_eq(response.headers[1].key, "Content-Length", "Second header key should be Content-Length");
  cr_assert_str_eq(response.headers[1].value, "25", "Content-Length should match body length");
  cr_assert(strlen(response.headers[1].key) < HTTP_HEADER_KEY_LEN, "Content-Length key should fit in buffer");
  cr_assert(strlen(response.headers[1].value) < HTTP_HEADER_VALUE_LEN, "Content-Length value should fit in buffer");

  cr_assert_str_eq(response.headers[2].key, "Content-Type", "Third header key should be Content-Type");
  cr_assert_str_eq(response.headers[2].value, "text/plain", "Content-Type should be text/plain");
  cr_assert(strlen(response.headers[2].key) < HTTP_HEADER_KEY_LEN, "Content-Type key should fit in buffer");
  cr_assert(strlen(response.headers[2].value) < HTTP_HEADER_VALUE_LEN, "Content-Type value should fit in buffer");

  free_http_response(&response);
}

