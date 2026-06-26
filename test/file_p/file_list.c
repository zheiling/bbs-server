#include <cmocka.h>
#include <db/db.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


#include <file_p.h>
#include <main.h>
#include <string.h>
#include <sys/cdefs.h>
#include <unistd.h>
#include <test_utils.h>

#define COUNT 20
dbuf_t *dbuf;
int amount_available = 0;

fl_t fl_sample = {.name = "test_file",
                  .description = "Hello test file",
                  .owner = "user1234",
                  .next = NULL};

ssize_t __wrap_write(int __fd, const void *__buf, size_t __n) __wur {
  dbuf_write(__buf, __n, &dbuf);
  return __n;
}

int32_t __wrap_db_get_files_data(i_get_files_db *arg, fl_t **fl_start,
                                 int32_t *full_count) {
  int amount = amount_available < arg->limit ? amount_available : arg->limit;
  fill_list_with_samples(&fl_sample, fl_start, NULL, amount);
  *full_count = amount;
  return amount;
}

void test__file_list(void **state) {
  session sess = {
      .fl_start = NULL,
      .fl_current = NULL,
      .uname = "user1234",
  };
  amount_available = 200;
  i_file_list_t fl_args = {.page = 1, .limit = COUNT, .name = NULL};
  dbuf = dbuf_init(INBUFSIZE);
  file_list(&sess, &fl_args);
  assert_int_not_equal(dbuf->len, 0);
  char *end_str = strstr(dbuf->ptr, ":END: PAGE");
  char strcmp[128];
  sprintf(strcmp, ":END: PAGE 1/1 COUNT: %d/%d\n", COUNT, COUNT);
  assert_string_equal(strcmp, end_str);
  dbuf_destroy(&dbuf);
}

void test__file_list__empty(void **state) {
  session sess = {
      .fl_start = NULL,
      .fl_current = NULL,
      .uname = "user1234",
  };
  amount_available = 0;
  i_file_list_t fl_args = {.page = 1, .limit = COUNT, .name = NULL};
  dbuf = dbuf_init(INBUFSIZE);
  file_list(&sess, &fl_args);
  assert_int_not_equal(dbuf->len, 0);
  char *end_str = strstr(dbuf->ptr, ":END: PAGE");
  char strcmp[128];
  sprintf(strcmp, ":END: PAGE 1/1 COUNT: %d/%d\n", 0, 0);
  assert_string_equal(strcmp, end_str);
  dbuf_destroy(&dbuf);
}

int setup(void **state) { return 0; }
int tear_down(void **state) { return 0; }

int main(int argc, char **argv) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test__file_list),
      cmocka_unit_test(test__file_list__empty),
  };

  return cmocka_run_group_tests(tests, setup, tear_down);
}
