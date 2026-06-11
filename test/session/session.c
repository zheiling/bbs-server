#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <session.h>
#include "../../src/main.h"

#define TEST_BUF "Hello world!\n"

int __wrap_read(int fd, void *buf, size_t nbytes) {
    return sizeof TEST_BUF - 1;
}
 
void test_session_do_read(void** state) { 
    session sess = {
        .buf = TEST_BUF,
        .buf_used = sizeof TEST_BUF,
        .sd = 2,
    };
    char *str;
    session_do_read(NULL, &str);
    assert_memory_equal(str, TEST_BUF, sizeof (TEST_BUF)-1);
}

int setup(void **state) { return 0; }
int tear_down(void **state) { return 0; }

int main(int argc, char **argv) {
  const struct CMUnitTest tests[] = {cmocka_unit_test(test_session_do_read)};

  return cmocka_run_group_tests(tests, setup, tear_down);
}
