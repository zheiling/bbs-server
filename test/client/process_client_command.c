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

int file_list_calls = 0;
int _limit = 15;
int _page = 2;
char _line[128];
char file_name[] = "hello_file";
int file_size = 121212;
int file_perm = 1;

void __wrap_file_list(session *sess, i_file_list_t *f_args) {
  assert_int_equal(_limit, f_args->limit);
  assert_int_equal(_page, f_args->page);
  file_list_calls++;
}

int32_t __wrap_file_send_prepare(session *sess, char *line,
                                 server_data_t *s_d) {
  assert_string_equal(line, _line);
  return mock_type(int);
}

int __wrap_file_receive_prepare(session *sess, char *line, server_data_t *s_d) {
  assert_string_equal(line, _line);
  return mock_type(int);
}

void test__process_client_command__file_list(void **state) {
  session sess = {};
  char line[128];

  sprintf(line, "file list %d %d", _limit, _page);
  process_client_command(line, &sess, NULL);
  assert_int_equal(file_list_calls, 1);
  file_list_calls = 0;
}

void test__process_client_command__file_upload(void **state) {
  session sess = {.state = OP_WAIT};

  will_return(__wrap_file_receive_prepare, 0);
  sprintf(_line, "file upload \"%s\" %d %d", file_name, file_size, file_perm);

  process_client_command(_line, &sess, NULL);

  assert_int_equal(file_list_calls, 0);
  assert_int_equal(sess.state, OP_UPLOAD);
}

void test__process_client_command__file_upload_fail(void **state) {
  session sess = {.state = OP_WAIT};

  will_return(__wrap_file_receive_prepare, 1);
  sprintf(_line, "file upload \"%s\" %d %d", file_name, file_size, file_perm);

  process_client_command(_line, &sess, NULL);

  assert_int_equal(file_list_calls, 0);
  assert_int_equal(sess.state, OP_WAIT);
}

int setup(void **state) { return 0; }
int tear_down(void **state) { return 0; }

int main(int argc, char **argv) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test__process_client_command__file_list),
      cmocka_unit_test(test__process_client_command__file_upload),
      cmocka_unit_test(test__process_client_command__file_upload_fail),
  };

  return cmocka_run_group_tests(tests, setup, tear_down);
}
