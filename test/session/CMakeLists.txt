# ---/ session_do_read /---------------------------------------------

add_executable(session_do_read
    session_do_read.c
)

target_link_libraries(session_do_read
    cmocka
    session
    main
)

target_link_options(session_do_read PRIVATE -Wl,--wrap=read)

add_test(NAME session_test_1 COMMAND session_do_read)

# ---/ query_extract_from_buf /--------------------------------------

add_executable(query_extract_from_buf
    query_extract_from_buf.c
)

target_link_libraries(query_extract_from_buf
    cmocka
    session
    main
)

add_test(NAME session_test_2 COMMAND query_extract_from_buf)