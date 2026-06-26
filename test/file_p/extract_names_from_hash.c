#include <cmocka.h>
#include <db/db.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


#include <file_p.h>
#include <main.h>
#include <string.h>
#include <sys/cdefs.h>
#include <unistd.h>

typedef struct {
  int32_t hash;
  char dir[3];
  char name[7];
} name_example_t;

name_example_t samples[] = {
    {.hash = 2138234, .dir = "00", .name = "20a07a"},
    {.hash = 123456, .dir = "00", .name = "01e240"},
    {.hash = 99399234, .dir = "05", .name = "ecb642"},
};

void test__extract_names_from_hash(void **state) {
    for (int i = 0 ; i < 3; i++) {
        char dir[3];
        char name[7];
        extract_names_from_hash(samples[i].hash, dir, name);
        assert_string_equal(dir, samples[i].dir);
        assert_string_equal(name, samples[i].name);
    }
}

int setup(void **state) { return 0; }
int tear_down(void **state) { return 0; }

int main(int argc, char **argv) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test__extract_names_from_hash),
  };

  return cmocka_run_group_tests(tests, setup, tear_down);
}
