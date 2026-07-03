#include <cmocka.h>
#include <db/db.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include <client.h>
#include <file_p.h>
#include <main.h>
#include <server.h>

int _errno = 0;
int _exit_code = 0;
extern void *print_str_arr;

/* TODO: finish */

int __wrap_socket(int __domain, int __type, int __protocol) {
  errno = _errno;
  return mock_type(int);
}

int __wrap_setsockopt(int __fd, int __level, int __optname,
                      const void *__optval, socklen_t __optlen) {
  errno = _errno;
  return mock_type(int);
}

int __wrap_bind(int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len) {
  errno = _errno;
  return mock_type(int);
}

int __wrap_listen(int __fd, int __n) {
  errno = _errno;
  return mock_type(int);
}

void __wrap_exit(int __status) { _exit_code = __status; }

void test__start_server_success(void **state) {
  will_return(__wrap_socket, 1);
  will_return(__wrap_setsockopt, 0);
  will_return(__wrap_bind, 0);
  will_return(__wrap_listen, 0);
  int ret = start_server();
  assert_int_equal(ret, 1);
}

void test__start_server_err_socket(void **state) {
  _errno = EACCES;
  will_return(__wrap_socket, -1);

  will_return(__wrap_setsockopt, 0);
  will_return(__wrap_bind, 0);
  will_return(__wrap_listen, 0);

  const char *fmt_args[] = {"starting, socket: %s\n", strerror(_errno), NULL};
  print_str_arr = fmt_args;
  start_server();
  assert_int_equal(_exit_code, 3);
}

void test__start_server_err_setsockopt(void **state) {
  _errno = EBADF;
  will_return(__wrap_socket, 1);
  will_return(__wrap_setsockopt, -1);
  will_return(__wrap_bind, 0);
  will_return(__wrap_listen, 0);

  const char *fmt_args[] = {"starting, setsockopt: %s\n", strerror(_errno),
                            NULL};
  print_str_arr = fmt_args;
  start_server();
  assert_int_equal(_exit_code, 4);
}

int setup(void **state) {
  print_str_arr = NULL;
  return 0;
}
int tear_down(void **state) { return 0; }

int main(int argc, char **argv) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test__start_server_success),
      cmocka_unit_test(test__start_server_err_socket),
  };

  return cmocka_run_group_tests(tests, setup, tear_down);
}
