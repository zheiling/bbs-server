#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <main.h>
#include <session.h>
#include <string.h>

#define TEST_BUF "First line\nThe second line\r\nThe third line\nFourth line\0"
#define SUBSTR_NUM 4

void test_query_extract_from_buf_filled(void **state) {
  session sess = {.buf = TEST_BUF, .buf_used = sizeof TEST_BUF};
  char *lines[SUBSTR_NUM];
  for (int i = 0; i < SUBSTR_NUM; i++) {
    query_extract_from_buf(&sess, &lines[i]);
  }
  assert_memory_equal("First line\n", lines[0], strlen(lines[0]));
  assert_memory_equal("The second line\n", lines[1], strlen(lines[1]));
  assert_memory_equal("The third line\n", lines[2], strlen(lines[2]));
  assert_memory_equal("Fourth line", lines[3], strlen(lines[3]));

  for (int i = 0; i < SUBSTR_NUM; i++) {
    free(lines[i]);
  }
}

void test_query_extract_from_buf_empty(void **state) {
  session sess = {.buf = "", .buf_used = 0};
  char *line = NULL;
  int ret = query_extract_from_buf(&sess, &line);
  assert_null(line);
  assert_int_equal(ret, 0);
}

int setup(void **state) { return 0; }
int tear_down(void **state) { return 0; }

int main(int argc, char **argv) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_query_extract_from_buf_filled),
      cmocka_unit_test(test_query_extract_from_buf_empty)
    };

  return cmocka_run_group_tests(tests, setup, tear_down);
}
