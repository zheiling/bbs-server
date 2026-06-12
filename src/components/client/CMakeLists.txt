add_library(client STATIC)

target_link_libraries(client 
    PRIVATE
        db
        main
        file_p
        session
        utils
        murmur3
        OpenSSL::Crypto
)

target_sources(client
    PRIVATE
        client.c
    PUBLIC
        FILE_SET HEADERS
        FILES
            client.h
)
