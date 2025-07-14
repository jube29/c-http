#include "../include/http.h"
#include <criterion/internal/test.h>
#include <string.h>

// Method Parsing

Test(http, should_parse_get) {
    cr_assert(parse_http_method("GET") == PARSE_OK, "GET method should be parsed");
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

Test(http, should_not_parse_too_long_request_line) {
    char line[4097];
    memset(line, 'a', 4096);
    line[4096] = '\0';
    http_request_t request = {0};
    cr_assert(parse_http_request(line, &request) == PARSE_UNTERMINATED_REQUEST_LINE, "Too long request line should not be parsed");
}

