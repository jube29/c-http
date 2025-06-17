cmake_minimum_required(VERSION 3.10)
project(chttp VERSION 0.1.0)
add_executable(chttp 
    src/main.c
    src/tcp.c
)

target_include_directories(chttp PRIVATE include)
target_link_libraries(chttp PRIVATE -lpthread)

install(TARGETS chttp DESTINATION bin)
