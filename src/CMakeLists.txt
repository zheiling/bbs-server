add_executable(zbbs-server)

set_property(TARGET zbbs-server PROPERTY C_STANDARD 23)

file(GLOB M_SOURCES "*.c")

target_sources(zbbs-server
    PRIVATE
        ${M_SOURCES}
)

add_library(main INTERFACE)

target_sources(main
    PUBLIC
        FILE_SET HEADERS
        FILES
            main.h
)


add_subdirectory(libs)
add_subdirectory(db)
add_subdirectory(components)

target_link_libraries(zbbs-server 
    PRIVATE
        session
        client
        db
        murmur3
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