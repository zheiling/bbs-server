cmake_minimum_required(VERSION 3.23)

project(ZBBSServer
    VERSION 1.0.0
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(
  (CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR
  (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
)
  add_compile_definitions(_GNU_SOURCE) 
endif()

find_package(PostgreSQL REQUIRED)
if(NOT PostgreSQL_FOUND)
    message(FATAL_ERROR "PostgreSQL not found. Please install libpq-dev or set PostgreSQL_ROOT.")
endif()

find_package(OpenSSL REQUIRED)
if(NOT OpenSSL_FOUND)
    message(FATAL_ERROR "OpenSSL not found. Please install libcrypto or set OpenSSL_ROOT.")
endif()


set(CURSES_NEED_NCURSES TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src) 

install(
  TARGETS zbbs-server
)