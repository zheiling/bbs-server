#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <main.h>
#include <session.h>
#include <string.h>

#define TEST_BUF_USED "Voluptas voluptate reprehenderit eum voluptatibus.\n"
#define TEST_BUF_UNUSED                                                        \
  "Impedit et rerum ut aut vel nisi. Animi et vero nihil aut possimus "        \
  "commodi. Autem dolorem et officia provident.\n"
#define TEST_BUF TEST_BUF_USED TEST_BUF_UNUSED

int __wrap_read(int fd, void *buf, size_t nbytes) {
  return sizeof TEST_BUF - 1;
}

void test_session_do_read(void **state) {
  session sess = {
      .buf = TEST_BUF,
      .buf_used = sizeof TEST_BUF,
      .sd = 2,
  };
  char *line;
  session_do_read(&sess, &line);
  assert_memory_equal(line, TEST_BUF_USED, sizeof(TEST_BUF_USED));
  free(line);
}

int setup(void **state) { return 0; }
int tear_down(void **state) { return 0; }

int main(int argc, char **argv) {
  const struct CMUnitTest tests[] = {cmocka_unit_test(test_session_do_read)};

  return cmocka_run_group_tests(tests, setup, tear_down);
}
