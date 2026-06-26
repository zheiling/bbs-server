add_library(test_utils STATIC)
target_link_libraries(test_utils cmocka main utils)
target_sources(test_utils
    PRIVATE
        test_utils.c
    PUBLIC
        FILE_SET HEADERS
        FILES
            test_utils.h
            )