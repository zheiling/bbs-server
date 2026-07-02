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

target_link_options(process_client_command PRIVATE -Wl,--wrap=file_list,--wrap=file_send_prepare,--wrap=file_receive_prepare,--wrap=session_send_string)

add_test(NAME client_process_client_command_test_1 COMMAND process_client_command)

# ---/ download_confirm /--------------------------------------------------

add_executable(download_confirm
    download_confirm.c
)

target_link_libraries(download_confirm
    cmocka
    client
    file_p
    main
    test_utils
)

target_link_options(download_confirm PRIVATE -Wl,--wrap=print_log,--wrap=clear_file_from_sess)

add_test(NAME client_download_confirm_test_1 COMMAND download_confirm)