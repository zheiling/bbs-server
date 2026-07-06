#include <main.h>
#include <cmocka.h>
#include <server.h>

/* TODO: finish */

int _errno = 0;
int _exit_code = 0;
extern void *print_str_arr;

void __wrap_exit(int __status) { _exit_code = __status; }

int __wrap_chdir(const char *__path) {
    return mock_type(int);
}

int __wrap_mkdir(const char *__path, __mode_t __mode) {
    return mock_type(int);
}

int __wrap_directory_exists(const char *path) {
    return mock_type(int);
}

void test__prepare_start__success(void **state) {
    int argc = 2;

    char *argv[] = {
        "test_prog",
        "w_dir"
    };

    will_return(__wrap_chdir, 0);
    will_return(__wrap_directory_exists, 1);

    prepare_start(argc,argv);
    assert_int_equal(_exit_code, 0);
}

void test__prepare_start__success_mkdir(void **state) {
    int argc = 2;

    char *argv[] = {
        "test_prog",
        "w_dir"
    };

    will_return(__wrap_chdir, 0);
    will_return(__wrap_directory_exists, 0);
    will_return(__wrap_mkdir, 1);

    prepare_start(argc,argv);
    assert_int_equal(_exit_code, 0);
}

void test__prepare_start__success_no_arg(void **state) {
    int argc = 1;

    char *argv[] = {
        "test_prog"
    };

    will_return(__wrap_chdir, 0);
    will_return(__wrap_directory_exists, 0);
    will_return(__wrap_mkdir, 1);

    prepare_start(argc,argv);
    assert_int_equal(_exit_code, 1);
}

int setup(void **state) {
  print_str_arr = NULL;
  return 0;
}
int tear_down(void **state) { return 0; }

int main(int argc, char **argv) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test__prepare_start__success),
      cmocka_unit_test(test__prepare_start__success_mkdir),
  };

  return cmocka_run_group_tests(tests, setup, tear_down);
}