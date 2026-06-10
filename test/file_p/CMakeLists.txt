add_executable(file_p_app
    file_p.c
)

target_link_libraries(file_p_app
    file_p
    Unity
)

add_test(file_p_test file_p_app)
