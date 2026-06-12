add_library(utils STATIC)

target_link_libraries(utils 
    PRIVATE
        client
        db
        murmur3
        OpenSSL::Crypto
)

target_sources(utils
    PRIVATE
        utils.c
    PUBLIC
        FILE_SET HEADERS
        FILES
            utils.h
)
