#include <main.h>
#include "cmocka.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_utils.h"
#include <utils.h>

dbuf_t *dbuf_init(size_t init_sz) {
  dbuf_t *dbuf = malloc(sizeof(dbuf_t));
  dbuf->ptr = malloc(init_sz);
  dbuf->len = 0;
  dbuf->capacity = init_sz;
  return dbuf;
}

static int it_times = 0;

int32_t dbuf_write(const char *text, size_t text_len, dbuf_t **__dbuf) {
  dbuf_t *dbuf = *__dbuf;
  if (text_len > (dbuf->capacity - dbuf->len)) {
    if (text_len > (dbuf->capacity * 2 - dbuf->len)) {
      dbuf->capacity = text_len * 2;
    } else {
      dbuf->capacity = dbuf->capacity * 2;
    }
    (*__dbuf)->ptr = realloc(dbuf->ptr, dbuf->capacity);
    dbuf = *__dbuf;
  }

  it_times++;
  int _it_times = it_times;
  strncpy(dbuf->ptr + dbuf->len, text, text_len);
  dbuf->ptr[dbuf->len + text_len] = '\0';
  dbuf->len += text_len;
  return 0;
}

int32_t dbuf_destroy(dbuf_t **dbuf) {
  free((*dbuf)->ptr);
  free(*dbuf);
  *dbuf = NULL;
  return 0;
}

void fill_list_with_samples(fl_t *fl_sample, fl_t **fl_st, fl_t **fl_cur,
                            int amount) {
  fl_t *_fl_st = NULL, *_fl_cur = NULL;

  if (fl_st == NULL)
    return;

  for (int i = 0; i < amount; i++) {
    if (i == 0) {
      _fl_st = _fl_cur = malloc(sizeof(fl_t));
      memcpy(_fl_st, fl_sample, sizeof(fl_t));
    } else {
      _fl_cur->next = malloc(sizeof(fl_t));
      memcpy(_fl_cur->next, fl_sample, sizeof(fl_t));
      _fl_cur = _fl_cur->next;
    }

    _fl_cur->next = NULL;

    _fl_cur->name = malloc(sizeof(char) * (strlen(fl_sample->name) + 32));
    _fl_cur->description =
        malloc(sizeof(char) * strlen(fl_sample->description) + 1);
    _fl_cur->owner = malloc(sizeof(char) * strlen(fl_sample->owner) + 1);

    sprintf(_fl_cur->name, "%s_%d", fl_sample->name, i + 1);
    memcpy(_fl_cur->description, fl_sample->description,
           sizeof(char) * strlen(fl_sample->description) + 1);
    memcpy(_fl_cur->owner, fl_sample->owner,
           sizeof(char) * strlen(fl_sample->owner) + 1);
  }
  *fl_st = _fl_st;
  if (fl_cur != NULL) {
    *fl_cur = _fl_cur;
  }
}

char *print_str = NULL;
void *print_str_arr = NULL;

int __wrap_print_log(FILE *output, enum pl_type type, const char *fmt, ...) {
  if (print_str != NULL) {
    assert_string_equal(print_str, fmt);
    print_str = NULL;
  } else if (print_str_arr != NULL) {
    char **ptrs = print_str_arr;
    va_list args;
    assert_string_equal(*ptrs, fmt);
    ptrs++;
    va_start(args, fmt);
    
    while (*ptrs != NULL) {
      char *out_str = va_arg(args, char *);
      assert_string_equal(out_str, *ptrs);
      ptrs++;
    }

    print_str_arr = NULL;

    va_end(args);
  }

  return 0;
};