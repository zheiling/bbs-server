#include <cmocka.h>
#include <db/db.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include <file_p.h>
#include <main.h>
#include <string.h>
/* TODO: complete  */
fl_t fl_sample = {
    .name = "test_file", .description = "Hello test file", .owner = "user1234"};

int32_t __wrap_db_get_files_data(i_get_files_db *arg, fl_t **fl_start,
                                 int32_t *full_count) {
  fill_list_with_samples(&fl_sample, fl_start, NULL, 123);
  *full_count = 123;
  return 123;
}

void test__file_list(void **state) {}

int setup(void **state) { return 0; }
int tear_down(void **state) { return 0; }

int main(int argc, char **argv) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test__file_list),
  };

  return cmocka_run_group_tests(tests, setup, tear_down);
}
