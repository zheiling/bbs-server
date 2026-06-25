# ---/ clear_list /--------------------------------------------------

add_executable(clear_list
    clear_list.c
    utils.c
)

target_link_libraries(clear_list
    cmocka
    file_p
    main
)

target_link_options(clear_list PRIVATE -Wl,--wrap=free)

add_test(NAME file_p_clear_list_test_1 COMMAND clear_list)

# ---/ file_list /---------------------------------------------------

add_executable(file_list
    file_list.c
    utils.c
)

target_link_libraries(file_list
    cmocka
    file_p
    main
)

target_link_options(file_list PRIVATE -Wl,--wrap=db_get_files_data,--wrap=write)

add_test(NAME file_p_file_list_test_1 COMMAND file_list)

# ---/ extract_names_from_hash /-------------------------------------

add_executable(extract_names_from_hash
    extract_names_from_hash.c
    utils.c
)

target_link_libraries(extract_names_from_hash
    cmocka
    file_p
    main
)

add_test(NAME file_p_extract_names_from_hash_test_1 COMMAND extract_names_from_hash)

# ---/ file_send_prepare /-------------------------------------------

add_executable(file_send_prepare
    file_send_prepare.c
    utils.c
)

target_link_libraries(file_send_prepare
    cmocka
    file_p
    main
    utils
)

target_link_options(file_send_prepare PRIVATE -Wl,--wrap=write,--wrap=print_log,--wrap=lseek,--wrap=open,--wrap=db_get_file)

add_test(NAME file_p_file_send_prepare_test_1 COMMAND file_send_prepare)

# ---/ file_receive_prepare /-------------------------------------------

add_executable(clear_file_from_sess
    clear_file_from_sess.c
    utils.c
)

target_link_libraries(clear_file_from_sess
    cmocka
    file_p
    main
    utils
)

target_link_options(clear_file_from_sess PRIVATE -Wl,--wrap=free)

add_test(NAME file_p_clear_file_from_sess_test_1 COMMAND clear_file_from_sess)

# ---/ file_download /-------------------------------------------

add_executable(file_download
    file_download.c
    utils.c
)

target_link_libraries(file_download
    cmocka
    file_p
    main
    utils
    client
)

target_link_options(file_download PRIVATE -Wl,--wrap=print_log,--wrap=read,--wrap=write)

add_test(NAME file_p_file_download_test_1 COMMAND file_download)