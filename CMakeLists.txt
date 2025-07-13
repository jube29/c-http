cmake_minimum_required(VERSION 3.10)
project(chttp VERSION 0.1.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(chttp 
    src/main.c
    src/tcp.c
    src/http.c
)

target_include_directories(chttp PRIVATE include)

add_executable(test_runner
    test/test_http.c
    src/tcp.c
    src/http.c
)

target_include_directories(test_runner PRIVATE include /usr/include/criterion)
target_link_libraries(test_runner PRIVATE pthread criterion)

install(TARGETS chttp DESTINATION bin)

