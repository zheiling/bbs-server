# ---/ process_client_command /--------------------------------------------------

add_executable(process_client_command
    process_client_command.c
)

target_link_libraries(process_client_command
    cmocka
    client
    file_p
    main
    test_utils
)

target_link_options(process_client_command PRIVATE -Wl,--wrap=file_list,--wrap=file_send_prepare,--wrap=file_receive_prepare)

add_test(NAME client_process_client_command_test_1 COMMAND process_client_command)