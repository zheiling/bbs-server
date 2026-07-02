#include <cmocka.h>
#include <db/db.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <file_p.h>
#include <main.h>
#include <test_utils.h>

fl_t fl_sample = {
    .name = "test_file", .description = "Hello test file", .owner = "user1234"};

int free_call_times = 0;

void __real_free(void *ptr);

void __wrap_free(void *ptr) {
  free_call_times++;
  __real_free(ptr);
}

#define CALL_TIMES 3

void test__clear_list__normal(void **state) {
  fl_t *fl_st, *fl_cur;
  free_call_times = 0;
  fill_list_with_samples(&fl_sample, &fl_st, &fl_cur, CALL_TIMES);

  clear_list(&fl_st, &fl_cur);
  assert_ptr_equal(fl_st, NULL);
  assert_ptr_equal(fl_cur, NULL);
  assert_int_equal(free_call_times, 4 * CALL_TIMES);
}

void test__clear_list__with__part_null(void **state) {
  fl_t *fl_st, *fl_cur;
  free_call_times = 0;
  fill_list_with_samples(&fl_sample, &fl_st, &fl_cur, CALL_TIMES);

  fl_cur->description = NULL;
  fl_cur->name = NULL;
  fl_cur->owner = NULL;

  clear_list(&fl_st, &fl_cur);
  assert_ptr_equal(fl_st, NULL);
  assert_ptr_equal(fl_cur, NULL);
  assert_int_equal(free_call_times, 4 * CALL_TIMES - 3);
}

void test__clear_list__with__null_1(void **state) {
  fl_t *fl_st = NULL, *fl_cur = NULL;
  free_call_times = 0;

  clear_list(&fl_st, &fl_cur);
  assert_ptr_equal(fl_st, NULL);
  assert_ptr_equal(fl_cur, NULL);
  assert_int_equal(free_call_times, 0);
}

void test__clear_list__with__cur_null(void **state) {
  fl_t *fl_st = NULL, *fl_cur = NULL;
  fill_list_with_samples(&fl_sample, &fl_st, &fl_cur, CALL_TIMES);

  clear_list(&fl_st, NULL);
  assert_ptr_equal(fl_st, NULL);
  assert_ptr_not_equal(fl_cur, NULL);
}

void test__clear_list__with__large(void **state) {
  fl_t *fl_st = NULL, *fl_cur = NULL;
  fill_list_with_samples(&fl_sample, &fl_st, &fl_cur, 1024);
  free_call_times = 0;

  clear_list(&fl_st, &fl_cur);
  assert_ptr_equal(fl_st, NULL);
  assert_ptr_equal(fl_cur, NULL);
  assert_int_equal(free_call_times, 1024 * 4);
}

int setup(void **state) { return 0; }
int tear_down(void **state) { return 0; }

int main(int argc, char **argv) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test__clear_list__normal),
      cmocka_unit_test(test__clear_list__with__part_null),
      cmocka_unit_test(test__clear_list__with__null_1),
      cmocka_unit_test(test__clear_list__with__cur_null),
      cmocka_unit_test(test__clear_list__with__large),
  };

  return cmocka_run_group_tests(tests, setup, tear_down);
}
