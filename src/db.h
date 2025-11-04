#ifndef DB_H
#define DB_H
#include "main.h"
#include <stdint.h>

/* Auth */

typedef struct {
  char *name;
  char *pass;
} i_auth_t;

typedef struct {
  char privileges;
  uint32_t uid;
} o_auth_t;

/* Get file */

typedef struct {
  int id;
  char *name;
  int user_id;
} i_get_file_db;

enum sort_direction { ASC, DESC };
enum sort_by { ID, NAME, CREATED_AT, USER_ID };

typedef struct {
  uint32_t user_id; // TODO: implement
  char name[12]; // TODO: implement
  uint32_t limit;
  uint32_t offset;
  enum sort_by sort_by;
  enum sort_direction sort_direction;
} i_get_files_db;

int init_db_connection();
int db_user_auth(i_auth_t *credentials, o_auth_t *response);
int db_save_file(session *s);
uint64_t db_get_files_data(i_get_files_db *arg, fl_t **fl_start,
                           uint64_t *full_count);
#endif