#include "db.h"
#include "main.h"
#include <libpq-fe.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED(x) (void)(x)
#define Q_LEN 128

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

int db_user_auth(i_auth_t *credentials, o_auth_t *response) {
  const char *paramValues[1];
  paramValues[0] = credentials->name;

  res = PQexecParams(conn,
                     "SELECT id, username, password, privileges "
                     "FROM users "
                     "WHERE username= $1",
                     1, NULL, paramValues, NULL, NULL, 0);

  if (PQresultStatus(res) != PGRES_TUPLES_OK && !PQntuples(res))
    return exit_query(1);

  const char *pass = PQgetvalue(res, 0, 2);
  if (!strcmp(pass, credentials->pass)) {
    response->privileges = PQgetvalue(res, 0, 3)[0];
    response->uid = atoi(PQgetvalue(res, 0, 0));
    return exit_query(0);
  }
  return exit_query(2);
}

int db_save_collision(uint32_t hash);

#define BIN 1
#define TEXT 0

int db_save_file(session *s) {
  const char *paramValues[6];
  int paramFormats[6];
  int paramLengths[6];
  int collision_id = 0;
  session_file *sf = s->file;

  int32_t uid_n = htonl(s->uid);
  int64_t size_n = htonl(sf->size);
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
  paramLengths[2] = sizeof(int64_t);
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
// int db_get_file(i_get_file_db *arg) {

// }

int db_get_files(i_get_files_db *arg) {
  const char *paramValues[3];
  int paramLengths[3];
  int paramFormats[3];
  int i, j, nFields;

  paramValues[0] = arg->sort_by;
  paramValues[1] = (char *)&arg->limit;
  paramValues[2] = (char *)&arg->offset;

  paramLengths[0] = strlen(arg->sort_by);
  paramLengths[1] = sizeof(arg->limit);
  paramLengths[2] = sizeof(arg->offset);

  paramFormats[0] = TEXT;
  paramFormats[1] = BIN;
  paramFormats[2] = BIN;

  res = PQexecParams(conn, "SELECT * FROM files ORDER BY $1 LIMIT $2 OFFSET $3",
                     3, NULL, paramValues, paramLengths, paramFormats, TEXT);

  nFields = PQnfields(res);
  for (i = 0; i < nFields; i++)
    printf("%-15s", PQfname(res, i));
  printf("\n\n");

  for (i = 0; i < PQntuples(res); i++) {
    for (j = 0; j < nFields; j++)
      printf("%-15s", PQgetvalue(res, i, j));
    printf("\n");
  }

  clearRes();

  return 0;
}