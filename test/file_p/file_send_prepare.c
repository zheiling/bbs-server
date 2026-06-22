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
#include <sys/cdefs.h>
#include <unistd.h>
#include <utils.h>

dbuf_t *dbuf;

#define FSIZE 10233
#define FNAME "Test file"
#define FHASH 211212
#define FID 19
#define FID_S "19"

int __wrap_print_log(FILE *output, enum pl_type type,
                     const char *format_message, ...) {
  return 0;
};

ssize_t __wrap_write(int __fd, const void *__buf, size_t __n) __wur {
  dbuf_write(__buf, __n, &dbuf);
  return __n;
}

__off_t __wrap_lseek(int __fd, __off_t __offset, int __whence) {
  return mock_type(int);
}

s_file_t *__wrap_db_get_file(i_get_file_db *arg) {
  assert_string_equal(arg->name, FNAME);
  return mock_type(s_file_t *);
}

int __wrap_open(const char *__file, int __oflag, ...) { return mock_type(int); }

void test__file_send_prepare(void **state) {
  s_file_t file = {.name = FNAME, .size = FSIZE, .hash = FHASH, .id = FID};
  char line[] = "[" FNAME "]";
  session sess = {
      .uname = "user1234",
      .sd = 0,
      .file = NULL,
  };
  int res = 0;

  will_return(__wrap_db_get_file, &file);
  will_return(__wrap_open, 5);
  will_return(__wrap_lseek, FSIZE);
  will_return(__wrap_lseek, 0);
  res = file_send_prepare(&sess, line, NULL);
  assert_int_equal(res, 0);
  assert_ptr_not_equal(sess.file, NULL);
  assert_int_equal(file.rest, FSIZE);
  assert_string_equal(file.path, "./storage/00/03390c");
}

void test__file_send_prepare__db_no_file(void **state) {
  char line[] = "[" FNAME "]";
  session sess = {
      .uname = "user1234",
      .sd = 0,
      .file = NULL,
  };
  int res = 0;

  will_return(__wrap_db_get_file, NULL);
  res = file_send_prepare(&sess, line, NULL);
  assert_int_equal(res, -1);
  assert_int_equal(sess.file, NULL);
}

void test__file_send_prepare__no_open(void **state) {
  dbuf = dbuf_init(INBUFSIZE);
  s_file_t file = {.name = FNAME, .size = FSIZE, .hash = FHASH, .id = FID};
  char line[] = "[" FNAME "]";
  session sess = {
      .uname = "user1234",
      .sd = 0,
      .file = NULL,
  };
  int res = 0;

  will_return(__wrap_db_get_file, &file);
  will_return(__wrap_open, -1);
  res = file_send_prepare(&sess, line, NULL);
  assert_int_equal(res, -2);
  assert_ptr_not_equal(sess.file, NULL);
  assert_string_equal("Can't open file with id = " FID_S "\n", dbuf->ptr);
  dbuf_destroy(&dbuf);
}

void test__file_send_prepare__size_not_correct(void **state) {
  dbuf = dbuf_init(INBUFSIZE);
  s_file_t file = {.name = FNAME, .size = FSIZE, .hash = FHASH, .id = FID};
  char line[] = "[" FNAME "]";
  session sess = {
      .uname = "user1234",
      .sd = 0,
      .file = NULL,
  };
  int res = 0;

  will_return(__wrap_db_get_file, &file);
  will_return(__wrap_open, 5);
  will_return(__wrap_lseek, FSIZE - 1);
  will_return(__wrap_lseek, 0);
  res = file_send_prepare(&sess, line, NULL);
  assert_int_equal(res, -3);
  assert_ptr_not_equal(sess.file, NULL);
  assert_string_equal(file.path, "./storage/00/03390c");
  assert_string_equal("File with id = " FID_S " exists, but seems to be damaged\n", dbuf->ptr);
  dbuf_destroy(&dbuf);
}

int setup(void **state) { return 0; }
int tear_down(void **state) { return 0; }

int main(int argc, char **argv) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test__file_send_prepare),
      cmocka_unit_test(test__file_send_prepare__no_open),
      cmocka_unit_test(test__file_send_prepare__db_no_file),
      cmocka_unit_test(test__file_send_prepare__size_not_correct),
  };

  return cmocka_run_group_tests(tests, setup, tear_down);
}
