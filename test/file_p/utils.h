#include "../../src/main.h"

typedef struct {
  char *ptr;
  size_t len;
  size_t capacity;
} dbuf_t;

void fill_list_with_samples(fl_t *fl_samples, fl_t **fl_st, fl_t **fl_cur,
                            int amount);
dbuf_t  *dbuf_init(size_t init_sz);
int32_t  dbuf_write(const char *text, size_t len, dbuf_t **dbuf);
int32_t  dbuf_destroy(dbuf_t **dbuf);