add_library(user STATIC)

target_link_libraries(user 
    PRIVATE
        client
        db
        murmur3
        main
        session
        OpenSSL::Crypto
)

target_sources(user
    PRIVATE
        user.c
    PUBLIC
        FILE_SET HEADERS
        FILES
            user.h
)
