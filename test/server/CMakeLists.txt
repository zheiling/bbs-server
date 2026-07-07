# ---/ start_server /------------------------------------------------

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
    server
    test_utils
)

target_link_options(start_server PRIVATE -Wl,--wrap=socket,--wrap=setsockopt,--wrap=bind,--wrap=listen,--wrap=exit,--wrap=print_log)

add_test(NAME client_start_server_test_1 COMMAND start_server)

# ---/ prepare_start /-----------------------------------------------

add_executable(prepare_start
    prepare_start.c
)

target_link_libraries(prepare_start
    cmocka
    client
    file_p
    session
    utils
    main
    server
    test_utils
)

target_link_options(prepare_start PRIVATE -Wl,--wrap=print_log,--wrap=chdir,--wrap=mkdir,--wrap=exit,--wrap=directory_exists)

add_test(NAME client_prepare_start_test_1 COMMAND prepare_start)

# ---/ server_main_loop /--------------------------------------------

add_executable(server_main_loop
    server_main_loop.c
)

target_link_libraries(server_main_loop
    cmocka
    client
    file_p
    session
    utils
    main
    server
    test_utils
)

target_link_options(server_main_loop PRIVATE -Wl,--wrap=print_log)

add_test(NAME client_server_main_loop_test_1 COMMAND server_main_loop)