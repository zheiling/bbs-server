cmake_minimum_required(VERSION 3.23)

project(ZBBSServer
    VERSION 1.0.0
)

add_subdirectory(src) 

install(
  TARGETS zbbs-server
)