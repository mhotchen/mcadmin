cmake_minimum_required(VERSION 3.5)
project(mcadmin)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c11 -x c -ansi")
link_libraries(ncurses)
link_libraries(panel)
link_libraries(cdk)

set(SOURCE_FILES main.c memcache/commands.h memcache/commands.c memcache/connect.c memcache/connect.h)
add_executable(mcadmin ${SOURCE_FILES})