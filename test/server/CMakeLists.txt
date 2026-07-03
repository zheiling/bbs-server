# ---/ start_server /--------------------------------------------------

add_executable(start_server
    start_server.c
)

target_link_libraries(start_server
    cmocka
    client
    file_p
    session
    utils
    main
    test_utils
)

target_link_options(start_server PRIVATE -Wl,--wrap=socket,--wrap=setsockopt,--wrap=bind,--wrap=listen,--wrap=exit)

add_test(NAME client_start_server_test_1 COMMAND start_server)