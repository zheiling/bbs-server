# ---/ create_user /-------------------------------------------------

add_executable(create_user
    create_user.c
)

target_link_libraries(create_user
    cmocka
    user
    main
    db
)

target_link_options(create_user PRIVATE -Wl,--wrap=db_user_create,--wrap=session_send_string)

add_test(NAME user_test_1 COMMAND create_user)

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
