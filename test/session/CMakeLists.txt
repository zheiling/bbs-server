add_executable(session_app
    session.c
)

target_link_libraries(session_app
    cmocka
    session
)

target_link_options(session_app PRIVATE -Wl,--wrap=read)

add_test(session_test session_app)
