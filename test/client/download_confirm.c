#include <cmocka.h>
#include <db/db.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <client.h>
#include <file_p.h>
#include <main.h>
#include <string.h>

#define PAC_SIZ 107898
#define FNAME "test_file_1"

char *str2snd = NULL;
int clear_file_from_sess_call = 0;

extern void *print_str_arr;

void __wrap_clear_file_from_sess(session *s) {
  clear_file_from_sess_call++;
  s->file = NULL;
  return;
}

void test__download_confirm__continue(void **state) {
  s_file_t file = {.package_rest = 0};
  session sess = {.state = OP_DOWNLOAD_WAIT_CONFIRM_PACKAGE, .file = &file};
  char line[128];
  sprintf(line, "continue %d\n", PAC_SIZ);

  download_confirm(line, &sess, NULL);
  assert_int_equal(sess.state, OP_DOWNLOAD);
  assert_int_equal(sess.file->package_rest, PAC_SIZ);
}

void test__download_confirm__continue_no_size(void **state) {
  s_file_t file = {.package_rest = 0};
  session sess = {.state = OP_DOWNLOAD_WAIT_CONFIRM_PACKAGE, .file = &file};
  char line[128];
  sprintf(line, "continue");

  download_confirm(line, &sess, NULL);
  assert_int_equal(sess.state, OP_DOWNLOAD);
  assert_int_equal(sess.file->package_rest, PACKAGE_SIZE);
}

void test__download_confirm__cancel(void **state) {
  s_file_t file = {.package_rest = 0, .name = FNAME};
  session sess = {.state = OP_DOWNLOAD_WAIT_CONFIRM_PACKAGE, .file = &file};
  const char *fmt_args[] = {"Downloading of %s is canceled by the client\n",
                            FNAME, NULL};
  print_str_arr = fmt_args;
  download_confirm("cancel\n", &sess, NULL);
  assert_int_equal(sess.state, OP_WAIT);
  assert_ptr_equal(sess.file, NULL);
}

void test__download_confirm__noise(void **state) {
  s_file_t file = {.package_rest = 0, .name = FNAME};
  session sess = {.state = OP_DOWNLOAD_WAIT_CONFIRM_PACKAGE, .file = &file};
  const char *fmt_args[] = {"The answer is not correct. Send either \"continue "
                            "\%PACKAGE_SIZE\% or \"cancel\"",
                            NULL};
  print_str_arr = fmt_args;
  download_confirm("jasdf", &sess, NULL);
  assert_int_equal(sess.state, OP_DOWNLOAD_WAIT_CONFIRM_PACKAGE);
}

int setup(void **state) { return 0; }
int tear_down(void **state) { return 0; }

int main(int argc, char **argv) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test__download_confirm__continue),
      cmocka_unit_test(test__download_confirm__continue_no_size),
      cmocka_unit_test(test__download_confirm__cancel),
      cmocka_unit_test(test__download_confirm__noise),
  };

  return cmocka_run_group_tests(tests, setup, tear_down);
}