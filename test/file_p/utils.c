#include "../../src/main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void fill_list_with_samples(fl_t *fl_sample, fl_t **fl_st,
                                   fl_t **fl_cur, int amount) {
  fl_t *_fl_st, *_fl_cur;
  
  if (fl_st == NULL) return;

  for (int i = 0; i < amount; i++) {
    if (i == 0) {
      _fl_st = _fl_cur = malloc(sizeof(fl_t));
      memcpy(_fl_st, fl_sample, sizeof(fl_t));
    } else {
      _fl_cur->next = malloc(sizeof(fl_t));
      memcpy(_fl_cur->next, fl_sample, sizeof(fl_t));
      _fl_cur = _fl_cur->next;
    }

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