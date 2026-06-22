#include <cmocka.h>
#include <db/db.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <main.h>
#include <string.h>
#include <user.h>

#define UNAME "user1234"
#define PASS "pass1234"
#define UID 1234
int id = 0;
char ret_buf[1024];

int32_t __wrap_db_user_auth(i_auth_t *c, o_auth_t *r) {
  r->uid = id;
  return id;
}

void __wrap_session_send_string(struct session *sess, const char *fmt, ...) {
  char buf[INBUFSIZE];
  size_t b_len = 0;
  va_list args;
  va_start(args, fmt);
  b_len = vsprintf(buf, fmt, args);
  strcpy(ret_buf, buf);
  va_end(args);
}

void test_create_login_success(void **state) {
  session sess = {.uname = UNAME};
  int ret = 0;
  id = UID;
  ret = login(&sess, PASS);
  assert_int_equal(ret, 0);
  assert_int_equal(sess.uid, UID);
  // assert_int_not_equal(sess.privileges, 0);
  assert_int_equal(sess.state, OP_WAIT);

  char welcome_mes[] = "Welcome, " UNAME "\n";
  assert_string_equal(welcome_mes, ret_buf);
}

void test_create_login_fail(void **state) {
  session sess = {.uname = UNAME};
  int ret = 0;
  id = 0;
  ret = login(&sess, PASS);
  assert_int_equal(ret, 1);
  assert_int_equal(sess.uid, 0);
  // assert_int_not_equal(sess.privileges, 0);
  assert_int_equal(sess.state, OP_LOGIN_USR);
  assert_string_equal("login_again>\n", ret_buf);
}

int setup(void **state) { return 0; }
int tear_down(void **state) { return 0; }

int main(int argc, char **argv) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_create_login_success),
      cmocka_unit_test(test_create_login_fail)
    };

  return cmocka_run_group_tests(tests, setup, tear_down);
}
