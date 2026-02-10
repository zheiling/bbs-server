add_executable(zbbs-server)

set_property(TARGET zbbs-server PROPERTY C_STANDARD 23)

find_package(PostgreSQL REQUIRED)
if(NOT PostgreSQL_FOUND)
    message(FATAL_ERROR "PostgreSQL not found. Please install libpq-dev or set PostgreSQL_ROOT.")
endif()

find_package(OpenSSL REQUIRED)
if(NOT OpenSSL_FOUND)
    message(FATAL_ERROR "OpenSSL not found. Please install libcrypto or set OpenSSL_ROOT.")
endif()

file(GLOB M_HEADERS "*.h")
file(GLOB M_SOURCES "*.c")

target_sources(zbbs-server
    PRIVATE
        ${M_SOURCES}
    PUBLIC
        FILE_SET HEADERS
        FILES
            ${M_HEADERS}
            "types.h"
)

add_subdirectory(libs)

target_link_libraries(zbbs-server 
    PRIVATE
        murmur3
        PostgreSQL::PostgreSQL
        OpenSSL::Crypto
)

if (ZBBS_DEBUG)
if(
  (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC") OR
  (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
)

  target_compile_options(zbbs-server PRIVATE /W3)
  message("Include debug info (Microsoft)")

elseif(
  (CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR
  (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
)

  target_compile_options(zbbs-server PRIVATE -Wall -ggdb -pedantic)
  message("Include debug info (GNU/Clang)")

endif()
endif()