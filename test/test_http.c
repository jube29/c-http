#include "../include/http.h"
#include <criterion/internal/test.h>
#include <stdio.h>
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
                            "Content-Type:  application/json  \r\n"
                            "Authorization:   Bearer token123   \r\n";
  parse_http_headers(raw_headers, &request);

  cr_assert_eq(request.headers_count, 3, "Should parse exactly 3 headers, got %d", (int)request.headers_count);

  cr_assert_str_eq(request.headers[0].key, "Host", "Expected key 'Host', got '%s'", request.headers[0].key);
  cr_assert_str_eq(request.headers[0].value, "example.com", "Expected value 'example.com', got '%s'",
                   request.headers[0].value);

  cr_assert_str_eq(request.headers[1].key, "Content-Type", "Expected key 'Content-Type', got '%s'",
                   request.headers[1].key);
  cr_assert_str_eq(request.headers[1].value, "application/json", "Expected value 'application/json', got '%s'",
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
                          "Accept: application/json\r\n"
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
  cr_assert_str_eq(request.headers[0].value, "example.com", "Expected header[0] value 'example.com', got '%s'", request.headers[0].value);
  
  // Check User-Agent header
  cr_assert_str_eq(request.headers[1].key, "User-Agent", "Expected header[1] key 'User-Agent', got '%s'", request.headers[1].key);
  cr_assert_str_eq(request.headers[1].value, "TestClient/1.0", "Expected header[1] value 'TestClient/1.0', got '%s'", request.headers[1].value);
  
  // Check Accept header
  cr_assert_str_eq(request.headers[2].key, "Accept", "Expected header[2] key 'Accept', got '%s'", request.headers[2].key);
  cr_assert_str_eq(request.headers[2].value, "application/json", "Expected header[2] value 'application/json', got '%s'", request.headers[2].value);
  
  // Check Authorization header
  cr_assert_str_eq(request.headers[3].key, "Authorization", "Expected header[3] key 'Authorization', got '%s'", request.headers[3].key);
  cr_assert_str_eq(request.headers[3].value, "Bearer token123", "Expected header[3] value 'Bearer token123', got '%s'", request.headers[3].value);
  
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
  cr_assert_str_eq(request.headers[0].value, "localhost", "Expected header value 'localhost', got '%s'", request.headers[0].value);
  
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

