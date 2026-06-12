add_library(server STATIC)

target_link_libraries(server 
    PRIVATE
        db
        main
        utils
        file_p
        session
        murmur3
        OpenSSL::Crypto
)

target_sources(server
    PRIVATE
        server.c
    PUBLIC
        FILE_SET HEADERS
        FILES
            server.h
)
