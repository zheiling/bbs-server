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
int _limit = DIF_LIMIT;
int _page = DIF_PAGE_NUM;
char *_name = NULL;
char _line[128];
char file_name[] = "hello_file";
int file_size = 121212;
int file_perm = 1;
char *str2snd = NULL;

void __wrap_session_send_string(struct session *sess, const char *fmt, ...) {
  if (str2snd != NULL) {
    assert_string_equal(fmt, str2snd);
    str2snd = NULL;
  }
}

void __wrap_file_list(session *sess, i_file_list_t *f_args) {
  assert_int_equal(_limit, f_args->limit);
  if (_limit != DIF_LIMIT) _limit = DIF_LIMIT;
  if (_page != DIF_PAGE_NUM) _page = DIF_PAGE_NUM;
  assert_int_equal(_page, f_args->page);
  if (_name != NULL) {
    assert_string_equal(_name, f_args->name);
    _name = NULL;
  }
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

void test__process_client_command__file_download(void **state) {
  session sess = {.state = OP_WAIT};

  will_return(__wrap_file_send_prepare, 0);
  sprintf(_line, "file download \"%s\" %d %d", file_name, file_size, file_perm);

  process_client_command(_line, &sess, NULL);

  assert_int_equal(sess.state, OP_DOWNLOAD);
}

void test__process_client_command__file_download_fail(void **state) {
  session sess = {.state = OP_WAIT};

  will_return(__wrap_file_send_prepare, 1);
  sprintf(_line, "file download \"%s\" %d %d", file_name, file_size, file_perm);

  process_client_command(_line, &sess, NULL);

  assert_int_equal(file_list_calls, 0);
  assert_int_equal(sess.state, OP_WAIT);
}

void test__process_client_command__file_search(void **state) {
  session sess = {.state = OP_WAIT};
  _name = "name";
  _limit = 20;
  _page = 2;

  sprintf(_line, "file search %s \"%s\" %d %d", _name, file_name, _limit, _page);

  process_client_command(_line, &sess, NULL);

  assert_int_equal(file_list_calls, 0);
  assert_int_equal(sess.state, OP_WAIT);
}

void test__process_client_command__file_search_no_page(void **state) {
  session sess = {.state = OP_WAIT};
  _name = "name";
  _limit = 20;

  sprintf(_line, "file search %s \"%s\" %d", _name, file_name, _limit);

  process_client_command(_line, &sess, NULL);

  assert_int_equal(file_list_calls, 0);
  assert_int_equal(sess.state, OP_WAIT);
}

void test__process_client_command__file_search_no_limit(void **state) {
  session sess = {.state = OP_WAIT};
  _name = "name";

  sprintf(_line, "file search %s \"%s\"", _name, file_name);

  process_client_command(_line, &sess, NULL);

  assert_int_equal(file_list_calls, 0);
  assert_int_equal(sess.state, OP_WAIT);
}

void test__process_client_command__file_search_no_name(void **state) {
  session sess = {.state = OP_WAIT};
  _name = "name";

  sprintf(_line, "file search name");
  str2snd = "The file name is not specified!";

  process_client_command(_line, &sess, NULL);

  assert_int_equal(file_list_calls, 0);
  assert_int_equal(sess.state, OP_WAIT);
}

void test__process_client_command__file_search_no_type(void **state) {
  session sess = {.state = OP_WAIT};

  sprintf(_line, "file search");
  str2snd = "The search criteria is not specified!";

  process_client_command(_line, &sess, NULL);

  assert_int_equal(file_list_calls, 0);
  assert_int_equal(sess.state, OP_WAIT);
}

void test__process_client_command__file(void **state) {
  session sess = {.state = OP_WAIT};

  sprintf(_line, "file");
  str2snd = "Available commands: list, upload, download, search";

  process_client_command(_line, &sess, NULL);

  assert_int_equal(file_list_calls, 0);
  assert_int_equal(sess.state, OP_WAIT);
}

void test__process_client_command__exit(void **state) {
  session sess = {.state = OP_WAIT};
  str2snd = "OK. Bye!\n";

  process_client_command("exit", &sess, NULL);

  assert_int_equal(file_list_calls, 0);
  assert_int_equal(sess.state, ERR);
  assert_int_equal(sess.reason, EXIT);
}

int setup(void **state) { return 0; }
int tear_down(void **state) { return 0; }

int main(int argc, char **argv) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test__process_client_command__file_list),
      cmocka_unit_test(test__process_client_command__file_upload),
      cmocka_unit_test(test__process_client_command__file_upload_fail),
      cmocka_unit_test(test__process_client_command__file_download),
      cmocka_unit_test(test__process_client_command__file_download_fail),
      cmocka_unit_test(test__process_client_command__file_search),
      cmocka_unit_test(test__process_client_command__file_search_no_page),
      cmocka_unit_test(test__process_client_command__file_search_no_limit),
      cmocka_unit_test(test__process_client_command__file_search_no_name),
      cmocka_unit_test(test__process_client_command__file_search_no_type),
      cmocka_unit_test(test__process_client_command__file),
      cmocka_unit_test(test__process_client_command__exit),
  };

  return cmocka_run_group_tests(tests, setup, tear_down);
}
