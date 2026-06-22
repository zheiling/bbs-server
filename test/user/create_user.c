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

#define UNAME "test_user"
#define PASS "1234"
#define EMAIL "test_user@test.com"

int32_t __wrap_db_user_create(i_db_user_create *args) { return mock_type(int); }

void __wrap_session_send_string(struct session *sess, const char *fmt, ...) {}

/* TODO: divide into separate cases and check messages */

void test__create_user__normal(void **state) {
  session sess = {};
  i_db_user_create p;
  int32_t ret = 0;
  char line[sizeof "register" + sizeof UNAME + sizeof PASS + sizeof EMAIL + 5];
  sprintf(line, "register %s %s %s\n", UNAME, PASS, EMAIL);
  /* Case: success */
  will_return(__wrap_db_user_create, 123);
  ret = create_user(&sess, line);
  assert_int_equal(ret, 123);
  /* Case: username exist */
  will_return(__wrap_db_user_create, -2);
  ret = create_user(&sess, line);
  assert_int_equal(ret, -2);
  /* Case: email exist */
  will_return(__wrap_db_user_create, -3);
  ret = create_user(&sess, line);
  assert_int_equal(ret, -3);
  /* Case: error */
  will_return(__wrap_db_user_create, -4);
  ret = create_user(&sess, line);
  assert_int_equal(ret, -4);
}

void test__create_user__partial(void **state) {
  session sess = {};
  i_db_user_create p;
  int32_t ret = 0;
  char line[1024] = "register " UNAME " " PASS "\n";

  /* Less args */
  ret = create_user(&sess, line);
  assert_int_equal(ret, 0);  

  /* More args */
  sprintf(line, "register %s %s %s not_used\n", UNAME, PASS, EMAIL);
  will_return(__wrap_db_user_create, 321);
  ret = create_user(&sess, line);
  assert_int_equal(ret, 321);
}

int setup(void **state) { return 0; }
int tear_down(void **state) { return 0; }

int main(int argc, char **argv) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test__create_user__normal),
      cmocka_unit_test(test__create_user__partial),
  };

  return cmocka_run_group_tests(tests, setup, tear_down);
}
