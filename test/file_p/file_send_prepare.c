#include <cmocka.h>
#include <db/db.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include <file_p.h>
#include <main.h>
#include <string.h>
#include <sys/cdefs.h>
#include <unistd.h>
#include <utils.h>

dbuf_t *dbuf;

/* TODO: finish */

int __wrap_print_log(FILE *output, enum pl_type type, const char *format_message,
              ...) {
  return 0;
};

ssize_t __wrap_write(int __fd, const void *__buf, size_t __n) __wur {
  dbuf_write(__buf, __n, &dbuf);
  return __n;
}

__off_t __wrap_lseek (int __fd, __off_t __offset, int __whence) __THROW {
  return mock_type(int);
}

int __wrap_open(const char *__file, int __oflag, ...) {
  return mock_type(int);
}

void test__file_send_prepare(void **state) {
  dbuf_t *dbuf = dbuf_init(INBUFSIZE);
  session sess = {
    .uname = "user1234",
    .sd = 0,
  };
}

int setup(void **state) { return 0; }
int tear_down(void **state) { return 0; }

int main(int argc, char **argv) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test__file_send_prepare),
  };

  return cmocka_run_group_tests(tests, setup, tear_down);
}
