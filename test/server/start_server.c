#include <cmocka.h>
#include <db/db.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>

#include <client.h>
#include <file_p.h>
#include <main.h>

/* TODO: finish */

int  __wrap_socket(int __domain, int __type, int __protocol) { return 0; }

int  __wrap_setsockopt(int __fd, int __level, int __optname,
                      const void *__optval, socklen_t __optlen) {
  return 0;
}

int  __wrap_bind(int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len) {
  return 0;
}

int  __wrap_listen(int __fd, int __n) { return 0; }

void __wrap_exit(int __status) {}

void test__start_server_success(void **state) {}

int setup(void **state) { return 0; }
int tear_down(void **state) { return 0; }

int main(int argc, char **argv) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test__start_server_success),
  };

  return cmocka_run_group_tests(tests, setup, tear_down);
}
