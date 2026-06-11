add_library(session STATIC)

target_link_libraries(session 
    PRIVATE
        client
        db
        murmur3
        OpenSSL::Crypto
)

target_sources(session
    PRIVATE
        session.c
    PUBLIC
        FILE_SET HEADERS
        FILES
            session.h
)
