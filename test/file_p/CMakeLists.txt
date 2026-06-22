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