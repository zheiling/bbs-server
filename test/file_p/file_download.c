#include "main.h"
#include <client.h>
#include <cmocka.h>
#include <errno.h>
#include <file_p.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <test_utils.h>
#include <utils.h>

dbuf_t *dbuf = NULL;
#define FNAME "Test file"
#define ITERATIONS 100
#define FSIZE PACKAGE_SIZE * ITERATIONS
int read_ret_zero = 0;
int write_ret_m_1 = 0;
extern char *print_str;
extern void *print_str_arr;

ssize_t __wrap_read(int __fd, void *__buf, size_t __nbytes) {
  if (read_ret_zero) {
    read_ret_zero = 0;
    return 0;
  }
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
  char cont_mes[64];

  sprintf(cont_mes, "continue %d\n", PACKAGE_SIZE);

  const char *fmt_args[] = {"File %s is downloaded from the server\n", FNAME,
                            NULL};
  print_str_arr = fmt_args;

  for (int i = 0; (sess.state != OP_WAIT) && (i < 2000000000); i++) {
    file_download(&sess);
    if (sess.state == OP_DOWNLOAD_WAIT_CONFIRM_PACKAGE) {
      download_confirm(cont_mes, &sess, NULL);
    }
  }
  assert_int_equal(sess.state, OP_WAIT);
  assert_ptr_equal(sess.file, NULL);
}

void test__file_download__normal_alt_buf_size(void **state) {
  dbuf = dbuf_init(INBUFSIZE);

  session sess = {.state = OP_DOWNLOAD};
  sess.file = malloc(sizeof(s_file_t));
  sess.file->name = malloc(sizeof FNAME);
  sess.file->size = sess.file->rest = FSIZE;
  sess.file->package_rest = PACKAGE_SIZE;
  sess.file->path = malloc(sizeof(char) * INBUFSIZE);
  sess.file->description = malloc(sizeof(char) * INBUFSIZE);
  strcpy(sess.file->name, FNAME);
  char cont_mes[64];

  sprintf(cont_mes, "continue %d\n", PACKAGE_SIZE / 3);

  const char *fmt_args[] = {"File %s is downloaded from the server\n", FNAME,
                            NULL};
  print_str_arr = fmt_args;

  for (int i = 0; (sess.state != OP_WAIT) && (i < 2000000000); i++) {
    file_download(&sess);
    if (sess.state == OP_DOWNLOAD_WAIT_CONFIRM_PACKAGE) {
      download_confirm(cont_mes, &sess, NULL);
    }
  }
  assert_int_equal(sess.state, OP_WAIT);
  assert_ptr_equal(sess.file, NULL);
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
    file_download(&sess);
    if (sess.state == OP_DOWNLOAD_WAIT_CONFIRM_PACKAGE) {
      download_confirm("cancel", &sess, NULL);
    }
  }
  assert_int_equal(sess.state, OP_WAIT);
  assert_ptr_equal(sess.file, NULL);
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
}

int setup(void **state) { return 0; }
int tear_down(void **state) { return 0; }

int main(int argc, char **argv) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test__file_download__normal),
      cmocka_unit_test(test__file_download__normal_alt_buf_size),
      cmocka_unit_test(test__file_download__cancel),
      cmocka_unit_test(test__file_download__read_zero),
      cmocka_unit_test(test__file_download__read_write_m_1),
  };

  return cmocka_run_group_tests(tests, setup, tear_down);
}
