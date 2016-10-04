cmake_minimum_required(VERSION 3.5)
project(mcadmin)
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)

link_libraries(
    ncurses
    panel
    cdk
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c11 -x c -ansi")

set(
    SOURCE_FILES
    src/main.c
    src/memcache/commands.h
    src/memcache/commands.c
    src/memcache/connect.c
    src/memcache/connect.h
    src/actions/actions.c
    src/actions/actions.h
    src/ui/modals.h
    src/ui/modals.c
    src/ui/screens.c
    src/ui/screens.h
)

add_executable(mcadmin ${SOURCE_FILES})
install(TARGETS mcadmin DESTINATION bin)
