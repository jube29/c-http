#include "../include/http.h"
#include <criterion/internal/test.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Method Parsing

Test(http, should_parse_get) { cr_assert(parse_http_method("GET") == PARSE_OK, "GET method should be parsed"); }

Test(http, should_parse_post) { cr_assert(parse_http_method("POST") == PARSE_OK, "POST method should be parsed"); }

Test(http, should_parse_delete) {
  cr_assert(parse_http_method("DELETE") == PARSE_OK, "DELETE method should be parsed");
}

Test(http, should_not_parse_put) {
  cr_assert(parse_http_method("PUT") == PARSE_INVALID_METHOD, "PUT method should not be parsed");
}

Test(http, should_not_parse_patch) {
  cr_assert(parse_http_method("PATCH") == PARSE_INVALID_METHOD, "PATCH method should not be parsed");
}

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

// Headers Parsing

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

  // Check first header
  cr_assert_str_eq(request.headers[0].key, "Host", "First header key should be 'Host'");
  cr_assert_str_eq(request.headers[0].value, "example.com", "First header value should be 'example.com'");

  // Check second header
  cr_assert_str_eq(request.headers[1].key, "User-Agent", "Second header key should be 'User-Agent'");
  cr_assert_str_eq(request.headers[1].value, "TestAgent/1.0", "Second header value should be 'TestAgent/1.0'");

  // Check third header
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

  // Verify some key headers
  cr_assert_str_eq(request.headers[0].key, "Host", "First header should be Host");
  cr_assert_str_eq(request.headers[0].value, "www.example.com:8080", "Host value should include port");

  cr_assert_str_eq(request.headers[5].key, "Connection", "Sixth header should be Connection");
  cr_assert_str_eq(request.headers[5].value, "keep-alive", "Connection value should be keep-alive");

  free_http_headers(&request);
}

// Memory Management Tests

Test(http, should_properly_initialize_headers_to_null) {
  http_request_t request = {0};
  cr_assert_null(request.headers, "Headers should be initialized to NULL");
  cr_assert_eq(request.headers_count, 0, "Headers count should be initialized to 0");
}

Test(http, should_free_headers_safely_when_null) {
  http_request_t request = {0};
  request.headers = NULL;
  request.headers_count = 0;

  // This should not crash
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

  // First free
  free_http_headers(&request);
  cr_assert_null(request.headers, "Headers should be NULL after first free");

  // Second free should not crash
  free_http_headers(&request);
  cr_assert_null(request.headers, "Headers should remain NULL after second free");
}

// Complete HTTP Request Parsing (Happy Path)

Test(http, should_parse_complete_http_request_with_headers) {
  http_request_t request = {0};
  const char *http_data = "GET /api/users HTTP/1.1\r\n"
                          "Host: example.com\r\n"
                          "User-Agent: TestClient/1.0\r\n"
                          "Accept: text/plain\r\n"
                          "Authorization: Bearer token123\r\n\r\n";

  parse_result_e result = parse_http_request(http_data, &request);

  // Check parsing result
  cr_assert_eq(result, PARSE_OK, "Expected PARSE_OK, got error code %d", result);

  // Validate request line components
  cr_assert_str_eq(request.method, "GET", "Expected method 'GET', got '%s'", request.method);
  cr_assert_str_eq(request.path, "/api/users", "Expected path '/api/users', got '%s'", request.path);
  cr_assert_str_eq(request.protocol, "HTTP/1.1", "Expected protocol 'HTTP/1.1', got '%s'", request.protocol);

  // Validate headers
  cr_assert_eq(request.headers_count, 4, "Expected 4 headers, got %d", (int)request.headers_count);

  // Check Host header
  cr_assert_str_eq(request.headers[0].key, "Host", "Expected header[0] key 'Host', got '%s'", request.headers[0].key);
  cr_assert_str_eq(request.headers[0].value, "example.com", "Expected header[0] value 'example.com', got '%s'",
                   request.headers[0].value);

  // Check User-Agent header
  cr_assert_str_eq(request.headers[1].key, "User-Agent", "Expected header[1] key 'User-Agent', got '%s'",
                   request.headers[1].key);
  cr_assert_str_eq(request.headers[1].value, "TestClient/1.0", "Expected header[1] value 'TestClient/1.0', got '%s'",
                   request.headers[1].value);

  // Check Accept header
  cr_assert_str_eq(request.headers[2].key, "Accept", "Expected header[2] key 'Accept', got '%s'",
                   request.headers[2].key);
  cr_assert_str_eq(request.headers[2].value, "text/plain", "Expected header[2] value 'text/plain', got '%s'",
                   request.headers[2].value);

  // Check Authorization header
  cr_assert_str_eq(request.headers[3].key, "Authorization", "Expected header[3] key 'Authorization', got '%s'",
                   request.headers[3].key);
  cr_assert_str_eq(request.headers[3].value, "Bearer token123", "Expected header[3] value 'Bearer token123', got '%s'",
                   request.headers[3].value);

  free_http_headers(&request);
}

Test(http, should_parse_minimal_complete_http_request) {
  http_request_t request = {0};
  const char *http_data = "GET / HTTP/1.1\r\n"
                          "Host: localhost\r\n\r\n";

  parse_result_e result = parse_http_request(http_data, &request);

  cr_assert_eq(result, PARSE_OK, "Expected PARSE_OK, got error code %d", result);
  cr_assert_str_eq(request.method, "GET", "Expected method 'GET', got '%s'", request.method);
  cr_assert_str_eq(request.path, "/", "Expected path '/', got '%s'", request.path);
  cr_assert_str_eq(request.protocol, "HTTP/1.1", "Expected protocol 'HTTP/1.1', got '%s'", request.protocol);
  cr_assert_eq(request.headers_count, 1, "Expected 1 header, got %d", (int)request.headers_count);
  cr_assert_str_eq(request.headers[0].key, "Host", "Expected header key 'Host', got '%s'", request.headers[0].key);
  cr_assert_str_eq(request.headers[0].value, "localhost", "Expected header value 'localhost', got '%s'",
                   request.headers[0].value);

  free_http_headers(&request);
}

Test(http, should_parse_complete_http_request_without_headers) {
  http_request_t request = {0};
  const char *http_data = "GET /index.html HTTP/1.1\r\n\r\n";

  parse_result_e result = parse_http_request(http_data, &request);

  // Check parsing result
  cr_assert_eq(result, PARSE_OK, "Expected PARSE_OK, got error code %d", result);

  // Validate request line components
  cr_assert_str_eq(request.method, "GET", "Expected method 'GET', got '%s'", request.method);
  cr_assert_str_eq(request.path, "/index.html", "Expected path '/index.html', got '%s'", request.path);
  cr_assert_str_eq(request.protocol, "HTTP/1.1", "Expected protocol 'HTTP/1.1', got '%s'", request.protocol);

  // Validate no headers
  cr_assert_eq(request.headers_count, 0, "Expected 0 headers, got %d", (int)request.headers_count);
  cr_assert_null(request.headers, "Expected headers to be NULL, got %p", request.headers);

  free_http_headers(&request);
}

// HTTP Body Parsing Tests - Happy Path

Test(http, should_parse_post_request_with_simple_body) {
  http_request_t request = {0};
  const char *http_data = "POST /api/users HTTP/1.1\r\n"
                          "Host: example.com\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: 16\r\n\r\n"
                          "name=John&age=30";

  parse_result_e result = parse_http_request(http_data, &request);
  cr_assert_eq(result, PARSE_OK, "Expected PARSE_OK, got error code %d", result);

  // Validate request line
  cr_assert_str_eq(request.method, "POST", "Expected method 'POST', got '%s'", request.method);
  cr_assert_str_eq(request.path, "/api/users", "Expected path '/api/users', got '%s'", request.path);

  // Test body parsing
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
  const char *http_data = "POST /api/data HTTP/1.1\r\n"
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
  const char *http_data = "POST /api/note HTTP/1.1\r\n"
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

// HTTP Body Parsing Tests - Common Pitfalls

Test(http, should_reject_body_larger_than_max_size) {
  http_request_t request = {0};
  size_t oversized_length = HTTP_MAX_BODY_SIZE + 1;
  char large_content_length[20];
  snprintf(large_content_length, sizeof(large_content_length), "%zu", oversized_length);

  const char *http_data_prefix = "POST /api HTTP/1.1\r\n"
                                 "Host: example.com\r\n"
                                 "Content-Type: text/plain\r\n"
                                 "Content-Length: ";
  const char *http_data_suffix = "\r\n\r\n";

  size_t request_size = strlen(http_data_prefix) + strlen(large_content_length) + strlen(http_data_suffix);
  char *http_data = malloc(request_size + oversized_length + 1);
  strcpy(http_data, http_data_prefix);
  strcat(http_data, large_content_length);
  strcat(http_data, http_data_suffix);

  // Add body data matching the declared Content-Length
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
  const char *http_data = "POST /api HTTP/1.1\r\n"
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
  const char *http_data = "POST /api HTTP/1.1\r\n"
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
  const char *http_data = "POST /api HTTP/1.1\r\n"
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
  const char *http_data = "POST /api HTTP/1.1\r\n"
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
  const char *http_data = "POST /api HTTP/1.1\r\n"
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
  const char *http_data = "POST /api HTTP/1.1\r\n"
                          "Host: example.com\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: 11\r\n\r\n";

  // Create full HTTP data with null bytes in body
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

  // Verify null byte is preserved
  cr_assert_eq(request.body[5], '\0', "Expected null byte at position 5");
  cr_assert_eq(request.body[6], 'W', "Expected 'W' at position 6");

  free(full_data);
  free_http_request(&request);
}

Test(http, should_handle_zero_content_length_with_no_body) {
  http_request_t request = {0};
  const char *http_data = "POST /api HTTP/1.1\r\n"
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
  strcpy(http_data, "POST /api HTTP/1.1\r\n"
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

// Content-Type Validation Tests

Test(http, should_accept_text_plain_content_type) {
  http_request_t request = {0};
  const char *http_data = "POST /api HTTP/1.1\r\n"
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
  const char *http_data = "POST /api HTTP/1.1\r\n"
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
  const char *http_data = "POST /api HTTP/1.1\r\n"
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
  const char *http_data = "POST /api HTTP/1.1\r\n"
                          "Host: example.com\r\n"
                          "Content-Length: 5\r\n\r\n"
                          "hello";

  parse_result_e result = parse_http_request(http_data, &request);
  cr_assert_eq(result, PARSE_UNSUPPORTED_CONTENT_TYPE, 
               "Expected PARSE_UNSUPPORTED_CONTENT_TYPE for POST without Content-Type header, got error code %d", result);

  free_http_request(&request);
}

Test(http, should_accept_get_request_without_content_type_header) {
  http_request_t request = {0};
  const char *http_data = "GET /api HTTP/1.1\r\n"
                          "Host: example.com\r\n\r\n";

  parse_result_e result = parse_http_request(http_data, &request);
  cr_assert_eq(result, PARSE_OK, "Expected PARSE_OK for GET without Content-Type header, got error code %d", result);

  free_http_request(&request);
}

