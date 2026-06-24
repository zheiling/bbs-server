#include <cmocka.h>
#include <db/db.h>
#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include "utils.h"
#include <file_p.h>
#include <main.h>
#include <string.h>
#include <sys/cdefs.h>
#include <unistd.h>
#include <utils.h>

#define FSIZE 10233
#define FSIZE_S "10233"
#define FNAME "Test file"
#define FHASH 211212
#define FID 19
#define FID_S "19"
#define ERR_FSV "Can't create a file."
#define STR_LONG                                                               \
  "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed sagittis "     \
  "risus et dictum consequat. Cras in fringilla dui. Aenean sed risus eu "     \
  "odio pellentesque tincidunt ac id nulla. Duis vel mi ut est finibus "       \
  "imperdiet. Praesent auctor convallis metus, quis ultricies dui. Praesent "  \
  "sit amet nibh quis elit mattis mollis. Nullam et mattis tortor, eu "        \
  "egestas nunc. Sed feugiat porttitor erat quis varius. Curabitur sapien "    \
  "est, consectetur consectetur nisi in, tempus gravida leo. Phasellus sit "   \
  "amet lorem a erat euismod sagittis eget vel libero. Suspendisse ac est "    \
  "aliquam, finibus lacus nec, commodo sapien. Orci varius natoque penatibus " \
  "et magnis dis parturient montes, nascetur ridiculus mus. Aliquam commodo, " \
  "nibh eget laoreet facilisis, nulla sapien mattis massa, ac luctus ex leo "  \
  "ut quam. Integer euismod velit in arcu porttitor suscipit. "

dbuf_t *dbuf;
size_t fs_available = 10;
size_t fs_namemax = 255;
int _errno = 0;

int __wrap_statvfs(const char *__restrict __file,
                   struct statvfs *__restrict __buf) {
  __buf->f_bavail = fs_available;
  __buf->f_bsize = 1024;
  __buf->f_namemax = fs_namemax;
  return 0;
}

int __wrap_stat(const char *__restrict __file, struct stat *__restrict __buf) {
  return mock_type(int);
}

int __wrap_mkdir(const char *__path, __mode_t __mode) { return mock_type(int); }

int __wrap_chdir(const char *__path) { return 0; }

int __wrap_print_log(FILE *output, enum pl_type type,
                     const char *format_message, ...) {
  return 0;
};

ssize_t __wrap_write(int __fd, const void *__buf, size_t __n) __wur {
  dbuf_write(__buf, __n, &dbuf);
  return __n;
}

int __wrap_open(const char *__file, int __oflag, ...) {
  errno = _errno;
  return mock_type(int);
}

/* CASE: Success */
void test__file_receive_prepare(void **state) {
  dbuf = dbuf_init(INBUFSIZE);
  char line[] = "\"" FNAME "\" " FSIZE_S " 1";
  session sess = {
      .uname = "user1234",
      .sd = 0,
      .file = NULL,
  };
  int res = 0;
  will_return(__wrap_mkdir, 0);
  will_return(__wrap_stat, 1);
  will_return(__wrap_open, 5);
  res = file_receive_prepare(&sess, line, NULL);
  assert_int_equal(res, 0);
  assert_string_equal("accept", dbuf->ptr);
  dbuf_destroy(&dbuf);
}

/* CASE: File name is too long */
void test__file_receive_prepare__name_too_long(void **state) {
  dbuf = dbuf_init(INBUFSIZE);
  char line[] = "\"" STR_LONG "\" " FSIZE_S " 1";
  session sess = {
      .uname = "user1234",
      .sd = 0,
      .file = NULL,
  };
  int res = 0;

  res = file_receive_prepare(&sess, line, NULL);
  assert_int_equal(res, -1);
  assert_string_equal("file name is too long\n", dbuf->ptr);
  assert_ptr_equal(sess.file, NULL);
  dbuf_destroy(&dbuf);
}

/* CASE: No enough space */
void test__file_receive_prepare__no_space(void **state) {
  dbuf = dbuf_init(INBUFSIZE);
  char line[] = "\"" FNAME "\" "
                "20000"
                " 1";
  session sess = {
      .uname = "user1234",
      .sd = 0,
      .file = NULL,
  };
  int res = 0;

  res = file_receive_prepare(&sess, line, NULL);
  assert_int_equal(res, -2);
  assert_string_equal("There is no space for such size! (20000)\n", dbuf->ptr);
  assert_ptr_equal(sess.file, NULL);
  dbuf_destroy(&dbuf);
}

/* CASE: Can't create directory */
void test__file_receive_prepare__err_dir(void **state) {
  dbuf = dbuf_init(INBUFSIZE);
  char line[] = "\"" FNAME "\" " FSIZE_S " 1";
  session sess = {
      .uname = "user1234",
      .sd = 0,
      .file = NULL,
  };
  int res = 0;
  will_return(__wrap_mkdir, 1);
  will_return(__wrap_stat, 1);
  res = file_receive_prepare(&sess, line, NULL);
  assert_int_equal(res, -4);
  assert_ptr_equal(sess.file, NULL);
  assert_string_equal(ERR_FSV, dbuf->ptr);
  dbuf_destroy(&dbuf);
}

/* CASE: Error in the file's name */
void test__file_receive_prepare__name_err(void **state) {
  dbuf = dbuf_init(INBUFSIZE);
  char line[] = "\"" FNAME "\" " FSIZE_S " 1";
  session sess = {
      .uname = "user1234",
      .sd = 0,
      .file = NULL,
  };
  int res = 0;
  will_return(__wrap_stat, 1);
  will_return(__wrap_mkdir, 0);
  will_return(__wrap_open, -1);
  _errno = EPERM;
  res = file_receive_prepare(&sess, line, NULL);
  assert_int_equal(res, -3);
  assert_ptr_equal(sess.file, NULL);
  assert_string_equal(ERR_FSV, dbuf->ptr);
  dbuf_destroy(&dbuf);
}

/* CASE: File with such name already exist */
void test__file_receive_prepare__exists(void **state) {
  dbuf = dbuf_init(INBUFSIZE);
  char line[] = "\"" FNAME "\" " FSIZE_S " 1";
  session sess = {
      .uname = "user1234",
      .sd = 0,
      .file = NULL,
  };
  int res = 0;

  /* 1st iteration */
  _errno = EEXIST;
  will_return(__wrap_stat, 1);
  will_return(__wrap_mkdir, 0);
  will_return(__wrap_open, -1);
  /* 2nd iteration */
  will_return(__wrap_stat, 1);
  will_return(__wrap_mkdir, 0);
  will_return(__wrap_open, 5);

  res = file_receive_prepare(&sess, line, NULL);
  assert_int_equal(res, 0);
  assert_ptr_not_equal(sess.file, NULL);
  assert_string_equal("accept", dbuf->ptr);
  dbuf_destroy(&dbuf);
}

int setup(void **state) { return 0; }
int tear_down(void **state) { return 0; }

int main(int argc, char **argv) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test__file_receive_prepare),
      cmocka_unit_test(test__file_receive_prepare__name_too_long),
      cmocka_unit_test(test__file_receive_prepare__no_space),
      cmocka_unit_test(test__file_receive_prepare__err_dir),
      cmocka_unit_test(test__file_receive_prepare__name_err),
      cmocka_unit_test(test__file_receive_prepare__exists),
  };

  return cmocka_run_group_tests(tests, setup, tear_down);
}
