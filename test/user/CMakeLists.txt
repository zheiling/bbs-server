# ---/ create_user /-------------------------------------------------

add_executable(test_create_user
    create_user.c
)

target_link_libraries(test_create_user
    cmocka
    user
    main
    db
)

target_link_options(test_create_user PRIVATE -Wl,--wrap=db_user_create,--wrap=session_send_string)

add_test(NAME user_test_1 COMMAND test_create_user)

# ---/ db_user_create /-------------------------------------------------

add_executable(db_user_create
    db_user_create.c
)

target_link_libraries(db_user_create
    cmocka
    user
    main
    db
)

target_link_options(db_user_create PRIVATE -Wl,--wrap=db_user_create,--wrap=session_send_string)

add_test(NAME user_test_2 COMMAND db_user_create)

# ---/ process_user_name /-------------------------------------------

add_executable(process_user_name
    process_user_name.c
)

target_link_libraries(process_user_name
    cmocka
    user
    main
)

target_link_options(process_user_name PRIVATE 
    -Wl,--wrap=session_send_string
)

add_test(NAME user_test_3 COMMAND process_user_name)

# ---/ login /-------------------------------------------------------

add_executable(login
    login.c
)

target_link_libraries(login
    cmocka
    user
    main
)

target_link_options(login PRIVATE 
    -Wl,--wrap=session_send_string,--wrap=db_user_auth
)

add_test(NAME user_test_4 COMMAND login)