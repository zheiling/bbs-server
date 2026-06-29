#include "main.h"
#include <client.h>
#include <cmocka.h>
#include <errno.h>
#include <file_p.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <test_utils.h>
#include <utils.h>

#define FNAME "Test file"
#define ITERATIONS 100
#define FSIZE PACKAGE_SIZE *ITERATIONS

dbuf_t *dbuf = NULL;
int read_ret_zero = 0;
int write_ret_m_1 = 0;
size_t package_rest = 0; /* indicator for the write function */
enum package_signal signal;
extern char *print_str;
extern void *print_str_arr;
char *str2snd;

void __wrap_session_send_string(struct session *sess, const char *fmt, ...) {
  assert_string_equal(fmt, str2snd);
}

int __wrap_unlink(const char *__name) { return 0; }

ssize_t __wrap_read(int __fd, void *__buf, size_t __nbytes) {
  if (read_ret_zero) {
    read_ret_zero = 0;
    return 0;
  }

  if (package_rest < __nbytes) {
    size_t cp_rest = __nbytes - package_rest;
    memset(__buf, 'e', package_rest);
    s_file_pd_t fpd = {.signal = signal, .package_size = PACKAGE_SIZE};
    memcpy(__buf + package_rest, &fpd, sizeof(s_file_pd_t));
    memset(__buf + package_rest + sizeof(s_file_pd_t), 'e', cp_rest);
    package_rest = PACKAGE_SIZE;
    package_rest -= cp_rest - sizeof(s_file_pd_t);
    size_t _package_rest = package_rest;
    return __nbytes;
  }

  package_rest -= __nbytes;
  memset(__buf, 'e', __nbytes);

  return __nbytes;
}

ssize_t __wrap_write(int __fd, const void *__buf, size_t __n) {
  if (write_ret_m_1)
    return -1;
  dbuf_write(__buf, __n, &dbuf);
  return __n;
}

int32_t __wrap_db_save_file(session *s) { return 1; }

void test__file_upload__success(void **state) {
  dbuf = dbuf_init(INBUFSIZE);

  session sess = {.state = OP_DOWNLOAD};
  sess.file = malloc(sizeof(s_file_t));
  sess.file->name = malloc(sizeof FNAME);
  sess.file->size = sess.file->rest = FSIZE;
  sess.file->package_rest = package_rest = PACKAGE_SIZE;
  sess.file->path = malloc(sizeof(char) * INBUFSIZE);
  sess.file->description = malloc(sizeof(char) * INBUFSIZE);
  strcpy(sess.file->name, FNAME);
  str2snd = "finished\n";

  const char *fmt_args[] = {"File %s is uploaded to the server\n", FNAME, NULL};
  print_str_arr = fmt_args;

  for (int i = 0; (sess.state != OP_WAIT) && (i < 2000000000); i++) {
    file_upload(&sess);
  }
  assert_int_equal(sess.state, OP_WAIT);
  assert_ptr_equal(sess.file, NULL);
}

void test__file_upload__no_read(void **state) {
  dbuf = dbuf_init(INBUFSIZE);

  session sess = {.state = OP_DOWNLOAD};
  sess.file = malloc(sizeof(s_file_t));
  sess.file->name = malloc(sizeof FNAME);
  sess.file->size = sess.file->rest = FSIZE;
  sess.file->package_rest = package_rest = PACKAGE_SIZE;
  sess.file->path = malloc(sizeof(char) * INBUFSIZE);
  sess.file->description = malloc(sizeof(char) * INBUFSIZE);
  strcpy(sess.file->name, FNAME);
  read_ret_zero = true;

  const char *fmt_args[] = {"Error uploading file %s!\n", FNAME, NULL};
  print_str_arr = fmt_args;

  for (int i = 0; (sess.state != OP_WAIT) && (i < 2000000000); i++) {
    file_upload(&sess);
  }
  assert_int_equal(sess.state, OP_WAIT);
  assert_ptr_equal(sess.file, NULL);
}

void test__file_upload__canceled(void **state) {
  dbuf = dbuf_init(INBUFSIZE);

  session sess = {.state = OP_DOWNLOAD};
  sess.file = malloc(sizeof(s_file_t));
  sess.file->name = malloc(sizeof FNAME);
  sess.file->size = sess.file->rest = FSIZE;
  sess.file->package_rest = package_rest = PACKAGE_SIZE;
  sess.file->path = malloc(sizeof(char) * INBUFSIZE);
  sess.file->description = malloc(sizeof(char) * INBUFSIZE);
  strcpy(sess.file->name, FNAME);

  const char *fmt_args[] = {"Upload of %s is cancelled!\n", FNAME, NULL};
  print_str_arr = fmt_args;

  for (int i = 0; (sess.state != OP_WAIT) && (i < 2000000000); i++) {
    if (i == 100)
      signal = sig_cancel;
    file_upload(&sess);
  }
  assert_int_equal(sess.state, OP_WAIT);
  assert_ptr_equal(sess.file, NULL);
  signal = sig_continue;
}

void test__file_upload__no_write_1(void **state) {
  dbuf = dbuf_init(INBUFSIZE);

  session sess = {.state = OP_DOWNLOAD};
  sess.file = malloc(sizeof(s_file_t));
  sess.file->name = malloc(sizeof FNAME);
  sess.file->size = sess.file->rest = FSIZE;
  sess.file->package_rest = package_rest = PACKAGE_SIZE;
  sess.file->path = malloc(sizeof(char) * INBUFSIZE);
  sess.file->description = malloc(sizeof(char) * INBUFSIZE);
  strcpy(sess.file->name, FNAME);
  str2snd = "There is an error sending the file named \"%s\"\n";

  const char *fmt_args[] = {"Error uploading the file \"%s\": %s\n", FNAME,
                            "No data available", NULL};
  print_str_arr = fmt_args;

  for (int i = 0; (sess.state != OP_WAIT) && (i < 2000000000); i++) {
    if (i == 100) {
      write_ret_m_1 = true;
      errno = ENODATA;
    }
    file_upload(&sess);
  }

  assert_int_equal(sess.state, OP_WAIT);
  assert_ptr_equal(sess.file, NULL);
}

void test__file_upload__no_write_2(void **state) {
  dbuf = dbuf_init(INBUFSIZE);

  session sess = {.state = OP_DOWNLOAD};
  sess.file = malloc(sizeof(s_file_t));
  sess.file->name = malloc(sizeof FNAME);
  sess.file->size = sess.file->rest = FSIZE;
  sess.file->package_rest = package_rest = PACKAGE_SIZE;
  sess.file->path = malloc(sizeof(char) * INBUFSIZE);
  sess.file->description = malloc(sizeof(char) * INBUFSIZE);
  strcpy(sess.file->name, FNAME);
  str2snd = "There is an error sending the file named \"%s\"\n";

  const char *fmt_args[] = {"Error uploading the file \"%s\": %s\n", FNAME,
                            "No data available", NULL};
  print_str_arr = fmt_args;

  for (int i = 0; (sess.state != OP_WAIT) && (i < 2000000000); i++) {
    if (i == 50) {
      signal = sig_cancel;
      write_ret_m_1 = true;
      errno = ENODATA;
    }
    file_upload(&sess);
  }

  assert_int_equal(sess.state, OP_WAIT);
  assert_ptr_equal(sess.file, NULL);
}

int setup(void **state) { return 0; }
int tear_down(void **state) { return 0; }

int main(int argc, char **argv) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test__file_upload__success),
      cmocka_unit_test(test__file_upload__no_read),
      cmocka_unit_test(test__file_upload__canceled),
      cmocka_unit_test(test__file_upload__no_write_1),
  };

  return cmocka_run_group_tests(tests, setup, tear_down);
}
