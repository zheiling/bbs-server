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