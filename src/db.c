#include "db.h"
#include "main.h"
#include <endian.h>
#include <fcntl.h>
#include <libpq-fe.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNUSED(x) (void)(x)
#define Q_LEN 128
#define BIN 1
#define TEXT 0
#define ENCR_SIZE 2048

static PGconn *conn = NULL;
static PGresult *res = NULL;

// #define USR_NOT_FOUND "User with such credentials is not found!"

static void clearRes() {
  PQclear(res);
  res = NULL;
}

static int terminate(int code) {
  if (code != 0)
    fprintf(stderr, "%s\n", PQerrorMessage(conn));

  if (res != NULL)
    PQclear(res);

  if (conn != NULL)
    PQfinish(conn);

  return code;
}

static int exit_query(int code) {
  if (code != 0)
    fprintf(stderr, "%s\n", PQerrorMessage(conn));

  if (res != NULL)
    PQclear(res);

  return code;
}

static int exit_query_2(int code) {
  if (code == 0)
    fprintf(stderr, "%s\n", PQerrorMessage(conn));

  if (res != NULL)
    PQclear(res);

  return code;
}

static void *exit_query_3(void *ptr) {
  fprintf(stderr, "%s\n", PQerrorMessage(conn));

  if (res != NULL)
    PQclear(res);

  return ptr;
}

static void processNotice(void *arg, const char *message) {
  UNUSED(arg);
  UNUSED(message);

  // do nothing
}

int init_db_connection() {
  // TODO: get from external configs
  conn = PQconnectdb("user=postgres password=dobro host=127.0.0.1 dbname=bbs");

  if (PQstatus(conn) != CONNECTION_OK)
    return terminate(1);

  int server_ver = PQserverVersion(conn);
  char *user = PQuser(conn);
  char *db_name = PQdb(conn);

  // res = PQexec(conn, "SELECT pg_catalog.set_config('search_path', '',
  // false)"); if (PQresultStatus(res) != PGRES_TUPLES_OK) {
  //   return terminate(2);
  // }

  printf("Connection established! server version: %d, user: %s, db: %s\n",
         server_ver, user, db_name);
  return exit_query(0);
}

void SHA256_raw_to_string(const unsigned char *passHashed, char *restrict out) {
  int i;
  for (i = 0; i < 4; i++) {
    uint64_t *num_pointer = (uint64_t *)(passHashed + i * 8);
    sprintf(out + 16 * i, "%016lx", htobe64(*num_pointer));
  }
}

void string_to_SHA256(const char *str, char *restrict out) {
  unsigned char md[SHA256_DIGEST_LENGTH];
  unsigned char *passHashed = SHA256((unsigned char *)str, strlen(str), md);
  SHA256_raw_to_string(passHashed, out);
}

int32_t db_user_auth(i_auth_t *c, o_auth_t *r) {
  const char *paramValues[1];
  paramValues[0] = c->name;
  char passHashed[SHA256_DIGEST_LENGTH * 2];

  res = PQexecParams(conn,
                     "SELECT id, username, password, privileges "
                     "FROM users "
                     "WHERE username= $1",
                     1, NULL, paramValues, NULL, NULL, 0);

  if (PQresultStatus(res) != PGRES_TUPLES_OK && !PQntuples(res))
    return exit_query(1);

  const char *pass = PQgetvalue(res, 0, 2);
  string_to_SHA256(c->pass, passHashed);
  if (pass == NULL) {
    /* TODO: return */
  }
  if (!strcmp(passHashed, pass)) {
    r->privileges = PQgetvalue(res, 0, 3)[0];
    r->uid = atoi(PQgetvalue(res, 0, 0));
    clearRes();
    char u_buf[128];
    sprintf(u_buf, "UPDATE users SET last_login = NOW() WHERE id = %u", r->uid);
    res = PQexec(conn, u_buf);
    if (PQresultStatus(res) != PGRES_TUPLES_OK && !PQntuples(res))
      return exit_query(3);
    return exit_query(0);
  }
  return exit_query(2);
}

int32_t db_user_create(i_db_user_create *args) {
  const char *paramValues[3];
  char passHashed[SHA256_DIGEST_LENGTH * 2];
  paramValues[0] = args->uname;
  paramValues[2] = args->email;
  uint32_t ret_value;

  string_to_SHA256(args->pass, passHashed);
  paramValues[1] = passHashed;

  res = PQexecParams(conn,
                     "INSERT INTO users (username, password, email, "
                     "privileges, created_at, last_login)"
                     " VALUES ($1, $2, $3, 1, NOW(), NOW()) RETURNING id",
                     3, NULL, paramValues, NULL, NULL, TEXT);

  if (PQresultStatus(res) != PGRES_TUPLES_OK && !PQntuples(res))
    return exit_query(-1);

  ret_value = atoi(PQgetvalue(res, 0, 0));
  clearRes();
  return ret_value;
}

int32_t db_save_file(session *s) {
  const char *paramValues[6];
  int paramFormats[6];
  int paramLengths[6];
  int collision_id = 0;
  s_file_t *sf = s->file;

  int32_t uid_n = htonl(s->uid);
  uint64_t size_n = htobe64(sf->size);
  int32_t hash_n = htonl(sf->hash);
  int32_t perm_n = htonl(sf->permissions);

  paramValues[0] = (char *)&uid_n;
  paramValues[1] = sf->name;
  paramValues[2] = (char *)&size_n;
  paramValues[3] = (char *)&hash_n;
  paramValues[4] = sf->description;
  paramValues[5] = (char *)&perm_n;

  paramLengths[0] = sizeof(int32_t);
  paramLengths[1] = strlen(sf->name);
  paramLengths[2] = sizeof(uint64_t);
  paramLengths[3] = sizeof(int32_t);
  paramLengths[4] = strlen(sf->description);
  paramLengths[5] = sizeof(int32_t);

  paramFormats[0] = BIN;
  paramFormats[1] = TEXT;
  paramFormats[2] = BIN;
  paramFormats[3] = BIN;
  paramFormats[4] = TEXT;
  paramFormats[5] = BIN;

  res = PQexecParams(conn,
                     "INSERT INTO files(user_id, name, size, created_at, "
                     "hash, description, permissions) "
                     "VALUES ($1, $2, $3, NOW(), $4, $5, $6) "
                     "RETURNING id",
                     6, NULL, paramValues, paramLengths, paramFormats, TEXT);

  if (PQresultStatus(res) != PGRES_TUPLES_OK && !PQntuples(res))
    return exit_query_2(0);

  int id = atoi(PQgetvalue(res, 0, 0));
  clearRes();
  return id;
}

// TODO: get file
s_file_t *db_get_file(i_get_file_db *arg) {
  s_file_t *sf;
  char s_field[24];
  char value[64] = "";
  const char *paramValues[1];
  char query[512];
  uint32_t q_len = 0;

  if (arg->id) {
    strcpy(s_field, "id");
    sprintf(value, "%u", arg->id);
  } else if (strlen(arg->name)) {
    strcpy(s_field, "name");
    strcpy(value, arg->name);
  } else if (arg->user_id) {
    strcpy(s_field, "user_id");
    sprintf(value, "%u", arg->user_id);
  } else {
    fprintf(stderr, "db_get_file: none of the args is specified!\n");
    return NULL;
  }

  paramValues[0] = value;

  sprintf(
      query,
      "SELECT files.id, user_id, name, size, description, permissions, hash "
      "FROM files JOIN users ON user_id = users.id WHERE files.%s = $1%n",
      s_field, &q_len);

  res = PQexecParams(conn, query, 1, NULL, paramValues, NULL, NULL, TEXT);

  if (PQresultStatus(res) != PGRES_TUPLES_OK && !PQntuples(res))
    return exit_query_3(NULL);

  sf = malloc(sizeof(s_file_t));
  sf->id = atoi(PQgetvalue(res, 0, 0));
  sf->owner_id = atoi(PQgetvalue(res, 0, 1));
  char *name = PQgetvalue(res, 0, 2);
  sf->name = malloc(strlen(name) + 1);
  strcpy(sf->name, name);
  sf->size = atoll(PQgetvalue(res, 0, 3));
  char *description = PQgetvalue(res, 0, 4);
  sf->description = malloc(strlen(description) + 1);
  strcpy(sf->description, description);
  sf->permissions = atoi(PQgetvalue(res, 0, 5));
  sf->hash = atoi(PQgetvalue(res, 0, 6));

  clearRes();

  return sf;
}

uint64_t db_get_files_data(i_get_files_db *arg, fl_t **fl_start,
                           uint64_t *full_count) {
  fl_t *fl_current;
  char query[512];
  char sort_by[16] = "id";
  char sort_dir[5] = "ASC";
  const char *paramValues[3];
  int paramLengths[3];
  int paramFormats[3];
  int i;
  uint64_t count;
  uint64_t n_limit = htobe64(arg->limit);
  uint64_t n_offset = htobe64(arg->offset);
  bool by_name = strlen(arg->name) > 0;
  uint32_t params_num = 3;

  switch (arg->sort_by) {
  case NAME:
    strcpy(sort_by, "name");
    break;
  case CREATED_AT:
    strcpy(sort_by, "created_at");
    break;
  case USER_ID:
    strcpy(sort_by, "user_id");
    break;
  case ID: // id is by default
  default:
    break;
  }

  if (arg->sort_direction == DESC) {
    strcpy(sort_dir, "DESC");
  }

  paramValues[0] = (char *)&n_limit;
  paramValues[1] = (char *)&n_offset;
  paramValues[2] = arg->name;

  paramLengths[0] = sizeof(n_limit);
  paramLengths[1] = sizeof(n_offset);
  paramLengths[2] = strlen(arg->name);

  paramFormats[0] = BIN;
  paramFormats[1] = BIN;
  paramFormats[2] = TEXT;

  if (!by_name) {
    params_num = 2;
    sprintf(query,
            "SELECT files.id, user_id, name, size, description, permissions, "
            "hash, username "
            "FROM files JOIN users ON user_id = users.id "
            "ORDER BY files.%s %s LIMIT $1 OFFSET $2",
            sort_by, sort_dir);
  } else {
    sprintf(query,
            "SELECT files.id, user_id, name, size, description, permissions, "
            "hash, username "
            "FROM files JOIN users ON user_id = users.id "
            "WHERE name ILIKE '%%' || $3 || '%%'"
            "ORDER BY files.%s %s LIMIT $1 OFFSET $2",
            sort_by, sort_dir);
  }

  res = PQexecParams(conn, query, params_num, NULL, paramValues, paramLengths,
                     paramFormats, TEXT);

  if (PQresultStatus(res) != PGRES_TUPLES_OK && !PQntuples(res))
    return exit_query_2(0);

  for (i = 0; i < PQntuples(res); i++) {
    fl_t *l_item = malloc(sizeof(fl_t));
    l_item->next = NULL;
    l_item->id = atoi(PQgetvalue(res, i, 0));
    l_item->owner_id = atoi(PQgetvalue(res, i, 1));
    char *name = PQgetvalue(res, i, 2);
    l_item->name = malloc(strlen(name) + 1);
    strcpy(l_item->name, name);
    l_item->size = atoll(PQgetvalue(res, i, 3));
    char *description = PQgetvalue(res, i, 4);
    l_item->description = malloc(strlen(description) + 1);
    strcpy(l_item->description, description);
    l_item->permissions = atoi(PQgetvalue(res, i, 5));
    l_item->hash = atoi(PQgetvalue(res, i, 6));
    char *owner = PQgetvalue(res, i, 7);
    l_item->owner = malloc(strlen(owner) + 1);
    strcpy(l_item->owner, owner);

    if (*fl_start == NULL) {
      *fl_start = l_item;
    } else {
      fl_current->next = l_item;
    }
    fl_current = l_item;
  }

  count = PQntuples(res);

  clearRes();

  if (by_name) {
    res = PQexecParams(conn,
                       "SELECT COUNT(id) "
                       "FROM files "
                       "WHERE name ILIKE '%%' || $1 || '%%'",
                       1, NULL, paramValues + 2, paramLengths + 2,
                       paramFormats + 2, TEXT);
  } else {
    res = PQexec(conn, "SELECT COUNT(id) "
                       "FROM files ");
  }

  if (PQresultStatus(res) != PGRES_TUPLES_OK && !PQntuples(res))
    return exit_query_2(0);

  *full_count = atoll(PQgetvalue(res, 0, 0));
  clearRes();
  return count;
}