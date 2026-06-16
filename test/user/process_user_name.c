#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <main.h>
#include <string.h>
#include <user.h>

static char user_name[] = "user1234";
char *str2snd;

void __wrap_session_send_string(struct session *sess, const char *fmt, ...) {
  assert_string_equal(fmt, str2snd);
}

int32_t create_user(session *sess, char *line) {
  return 321;
}

void test__process_user_name__user_name(void **state) {
  session sess = {.uname = NULL, .state = OP_WAIT};
  str2snd = "password> ";
  int ret = process_user_name(user_name, &sess);
  assert_int_equal(ret, 0);
  assert_string_equal(sess.uname, user_name);
  assert_int_equal(sess.state, OP_LOGIN_PSS);
}

void test__process_user_name__exit(void **state) {
  session sess = {.uname = NULL, .state = OP_WAIT};
  str2snd = "Bye!";
  int ret = process_user_name("exit", &sess);
  assert_int_equal(ret, 1);
  assert_int_equal(sess.state, ERR);
  assert_int_equal(sess.reason, EXIT);
}

void test__process_user_name__register(void **state) {
  session sess = {.uname = NULL, .state = OP_WAIT};
  str2snd = "ok\n";
  int ret = process_user_name("register", &sess);
  assert_int_equal(ret, 321);
}

void test__process_user_name__anonymous(void **state) {
  session sess = {.uname = NULL, .state = OP_WAIT};
  str2snd = "Welcome, Anonymous!";
  int ret = process_user_name("anonymous", &sess);
  assert_int_equal(ret, 3);
  assert_string_equal(sess.uname, "anonymous");
  assert_int_equal(sess.state, OP_WAIT);
}

int setup(void **state) { return 0; }
int tear_down(void **state) { return 0; }

int main(int argc, char **argv) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test__process_user_name__user_name),
      cmocka_unit_test(test__process_user_name__exit),
      cmocka_unit_test(test__process_user_name__register),
      cmocka_unit_test(test__process_user_name__anonymous),
    };

  return cmocka_run_group_tests(tests, setup, tear_down);
}
