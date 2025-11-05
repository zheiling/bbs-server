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
  uint32_t id;
  uint32_t user_id;
  char name[64];
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

int32_t init_db_connection();
int32_t db_save_file(session *s);
int32_t db_user_auth(i_auth_t *credentials, o_auth_t *response);
s_file_t *db_get_file(i_get_file_db *arg);
uint64_t db_get_files_data(i_get_files_db *arg, fl_t **fl_start,
                           uint64_t *full_count);
#endif