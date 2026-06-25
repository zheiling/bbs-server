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

int free_call_times = 0;

void __real_free(void *ptr);

void __wrap_free(void *ptr) {
  free_call_times++;
  __real_free(ptr);
}

void test__clear_file_from_sess__normal(void **state) {
  session sess = {};
  free_call_times = 0;
  s_file_t *f = malloc(sizeof(s_file_t));
  f->description = malloc(sizeof (char) * INBUFSIZE);
  f->name = malloc(sizeof (char) * INBUFSIZE);
  f->path = malloc(sizeof (char) * INBUFSIZE);

  sess.file = f;

  clear_file_from_sess(&sess);
  
  assert_ptr_equal(sess.file, NULL);
  assert_int_equal(free_call_times, 4);
}

void test__clear_file_from_sess__no_description(void **state) {
  session sess = {};
  free_call_times = 0;
  s_file_t *f = malloc(sizeof(s_file_t));
  f->description = NULL;
  f->name = malloc(sizeof (char) * INBUFSIZE);
  f->path = malloc(sizeof (char) * INBUFSIZE);

  sess.file = f;

  clear_file_from_sess(&sess);
  
  assert_ptr_equal(sess.file, NULL);
  assert_int_equal(free_call_times, 3);
}

void test__clear_file_from_sess__no_name(void **state) {
  session sess = {};
  free_call_times = 0;
  s_file_t *f = malloc(sizeof(s_file_t));
  f->description = malloc(sizeof (char) * INBUFSIZE);
  f->name = NULL;
  f->path = malloc(sizeof (char) * INBUFSIZE);

  sess.file = f;

  clear_file_from_sess(&sess);
  
  assert_ptr_equal(sess.file, NULL);
  assert_int_equal(free_call_times, 3);
}

void test__clear_file_from_sess__no_path(void **state) {
  session sess = {};
  free_call_times = 0;
  s_file_t *f = malloc(sizeof(s_file_t));
  f->description = malloc(sizeof (char) * INBUFSIZE);
  f->name = malloc(sizeof (char) * INBUFSIZE);
  f->path = NULL;

  sess.file = f;

  clear_file_from_sess(&sess);
  
  assert_ptr_equal(sess.file, NULL);
  assert_int_equal(free_call_times, 3);
}

void test__clear_file_from_sess__no_file(void **state) {
  session sess = {.file = NULL};
  free_call_times = 0;

  clear_file_from_sess(&sess);
  
  assert_ptr_equal(sess.file, NULL);
  assert_int_equal(free_call_times, 0);
}

int setup(void **state) { return 0; }
int tear_down(void **state) { return 0; }

int main(int argc, char **argv) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test__clear_file_from_sess__normal),
      cmocka_unit_test(test__clear_file_from_sess__no_description),
      cmocka_unit_test(test__clear_file_from_sess__no_name),
      cmocka_unit_test(test__clear_file_from_sess__no_path),
      cmocka_unit_test(test__clear_file_from_sess__no_file),
  };

  return cmocka_run_group_tests(tests, setup, tear_down);
}
