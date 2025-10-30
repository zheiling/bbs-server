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

typedef struct {
    int user_id; // TODO: implement
    char *name; // TODO: implement
    int limit;
    int offset;
    char sort_by[12];
} i_get_files_db;


int init_db_connection();
int db_user_auth(i_auth_t *credentials, o_auth_t *response);
int db_save_file(session *s);
