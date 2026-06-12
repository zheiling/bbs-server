add_library(file_p STATIC)

target_link_libraries(file_p 
    PRIVATE
        db
        main
        murmur3
        utils
        session
        OpenSSL::Crypto
)

target_sources(file_p
    PRIVATE
        file_p.c
    PUBLIC
        FILE_SET HEADERS
        FILES
            file_p.h
)
