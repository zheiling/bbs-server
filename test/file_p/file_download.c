#include "main.h"
#include "utils.h"
#include <client.h>
#include <cmocka.h>
#include <errno.h>
#include <file_p.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils.h>

dbuf_t *dbuf = NULL;
#define FNAME "Test file"
#define ITERATIONS 100
#define FSIZE PACKAGE_SIZE *ITERATIONS
int read_ret_zero = 0;
int write_ret_m_1 = 0;
char *print_str = NULL;
void *print_str_arr = NULL;

int __wrap_print_log(FILE *output, enum pl_type type, const char *fmt, ...) {
  if (print_str != NULL) {
    assert_string_equal(print_str, fmt);
  } else if (print_str_arr != NULL) {
    char **ptrs = print_str_arr;
    va_list args;
    assert_string_equal(*ptrs, fmt);
    ptrs++;
    va_start(args, fmt);
    while (*ptrs != NULL) {
      char *out_str = va_arg(args, char *);
      assert_string_equal(out_str, *ptrs);
      ptrs++;
    }

    va_end(args);
  }

  return 0;
};

ssize_t __wrap_read(int __fd, void *__buf, size_t __nbytes) {
  if (read_ret_zero)
    return 0;
  ssize_t _size = __nbytes;
  memset(__buf, 'e', __nbytes);
  return __nbytes;
}

ssize_t __wrap_write(int __fd, const void *__buf, size_t __n) {
  if (write_ret_m_1)
    return -1;
  dbuf_write(__buf, __n, &dbuf);
  return __n;
}

void test__file_download__normal(void **state) {
  dbuf = dbuf_init(INBUFSIZE);

  session sess = {.state = OP_DOWNLOAD};
  sess.file = malloc(sizeof(s_file_t));
  sess.file->name = malloc(sizeof FNAME);
  sess.file->size = sess.file->rest = FSIZE;
  sess.file->package_rest = PACKAGE_SIZE;
  sess.file->path = malloc(sizeof(char) * INBUFSIZE);
  sess.file->description = malloc(sizeof(char) * INBUFSIZE);
  strcpy(sess.file->name, FNAME);

  const char *fmt_args[] = {"File %s is downloaded from the server\n", FNAME,
                            NULL};
  print_str_arr = fmt_args;

  while (sess.state != OP_WAIT) {
    file_download(&sess);
    if (sess.state == OP_DOWNLOAD_WAIT_CONFIRM_PACKAGE) {
      download_confirm("continue", &sess, NULL);
    }
  }
  assert_int_equal(sess.state, OP_WAIT);
  assert_ptr_equal(sess.file, NULL);
  print_str_arr = NULL;
  clear_file_from_sess(&sess);
}

void test__file_download__cancel(void **state) {
  dbuf = dbuf_init(INBUFSIZE);

  session sess = {.state = OP_DOWNLOAD};
  sess.file = malloc(sizeof(s_file_t));
  sess.file->name = malloc(sizeof FNAME);
  sess.file->size = sess.file->rest = FSIZE;
  sess.file->package_rest = PACKAGE_SIZE;
  sess.file->path = malloc(sizeof(char) * INBUFSIZE);
  sess.file->description = malloc(sizeof(char) * INBUFSIZE);
  strcpy(sess.file->name, FNAME);

  const char *fmt_args[] = {"Downloading of %s is canceled by the client\n",
                            FNAME, NULL};
  print_str_arr = fmt_args;

  while (sess.state != OP_WAIT) {
    read_ret_zero = false;
    file_download(&sess);
    if (sess.state == OP_DOWNLOAD_WAIT_CONFIRM_PACKAGE) {
      download_confirm("cancel", &sess, NULL);
    }
  }
  assert_int_equal(sess.state, OP_WAIT);
  assert_ptr_equal(sess.file, NULL);
  print_str_arr = NULL;
}

void test__file_download__read_zero(void **state) {
  dbuf = dbuf_init(INBUFSIZE);

  session sess = {.state = OP_DOWNLOAD};
  sess.file = malloc(sizeof(s_file_t));
  sess.file->name = malloc(sizeof FNAME);
  sess.file->size = sess.file->rest = FSIZE;
  sess.file->package_rest = PACKAGE_SIZE;
  sess.file->description = NULL;
  sess.file->path = NULL;
  strcpy(sess.file->name, FNAME);
  const char *fmt_args[] = {"Error downloading file %s!\n", FNAME, NULL};
  print_str_arr = fmt_args;
  read_ret_zero = true;
  file_download(&sess);
  assert_int_equal(sess.state, OP_WAIT);
  assert_ptr_equal(sess.file, NULL);
  read_ret_zero = false;
  print_str_arr = NULL;
}

void test__file_download__read_write_m_1(void **state) {
  dbuf = dbuf_init(INBUFSIZE);

  session sess = {.state = OP_DOWNLOAD};
  sess.file = malloc(sizeof(s_file_t));
  sess.file->name = malloc(sizeof FNAME);
  sess.file->size = sess.file->rest = FSIZE;
  sess.file->package_rest = PACKAGE_SIZE;
  sess.file->description = NULL;
  sess.file->path = NULL;
  strcpy(sess.file->name, FNAME);

  const char *fmt_args[] = {"Error downloading file %s! %s\n", FNAME,
                            "Operation not permitted", NULL};
  print_str_arr = fmt_args;

  write_ret_m_1 = true;
  errno = EPERM;
  file_download(&sess);
  assert_int_equal(sess.state, ERR);
  assert_ptr_equal(sess.file, NULL);
  write_ret_m_1 = false;
  print_str_arr = NULL;
}

int setup(void **state) { return 0; }
int tear_down(void **state) { return 0; }

int main(int argc, char **argv) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test__file_download__normal),
      cmocka_unit_test(test__file_download__cancel),
      cmocka_unit_test(test__file_download__read_zero),
      cmocka_unit_test(test__file_download__read_write_m_1),
  };

  return cmocka_run_group_tests(tests, setup, tear_down);
}
