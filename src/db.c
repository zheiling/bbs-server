#include "db.h"
#include <libpq-fe.h>
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

static void processNotice(void *arg, const char *message) {
  UNUSED(arg);
  UNUSED(message);

  // do nothing
}

int init_connection() {
  // TODO: get from external configs
  conn = PQconnectdb("user=postgres password=dobro host=127.0.0.1 dbname=bbs");

  if (PQstatus(conn) != CONNECTION_OK)
    return terminate(1);

  int server_ver = PQserverVersion(conn);
  char *user = PQuser(conn);
  char *db_name = PQdb(conn);

  printf("Connection established! server version: %d, user: %s, db: %s\n", server_ver,
         user, db_name);
  return exit_query(0);
}

int db_user_auth(i_auth_t *credentials, o_auth_t *response) {
  char query[Q_LEN];
  response->is_logged = 0;
  sprintf(query,
          "SELECT username, password, privileges "
          "FROM users "
          "WHERE username= '%s'",
          credentials->name);
  res = PQexec(conn, query);

  if (PQresultStatus(res) != PGRES_TUPLES_OK && !PQntuples(res))
    return exit_query(1);

  const char *pass = PQgetvalue(res, 0, 1);
  if (!strcmp(pass, credentials->pass)) {
    response->is_logged = 1;
    response->privileges = PQgetvalue(res, 0, 2)[0];
    return exit_query(0);
  }
  return exit_query(2);
}

/*s

int f_main() {
  int libpq_ver = PQlibVersion();
  printf("Version of libpq: %d\n", libpq_ver);

  conn = PQconnectdb("user=postgres password=dobro host=127.0.0.1 dbname=bbs");
  if (PQstatus(conn) != CONNECTION_OK)
    terminate(1);

  PQsetNoticeProcessor(conn, processNotice, NULL);

  int server_ver = PQserverVersion(conn);
  char *user = PQuser(conn);
  char *db_name = PQdb(conn);

  printf("Server version: %d\n", server_ver);
  printf("User: %s\n", user);
  printf("Database name: %s\n", db_name);

  res = PQexec(conn, "SELECT id, username, privileges, email "
                     "FROM users");
  if (PQresultStatus(res) != PGRES_TUPLES_OK)
    terminate(1);

  int nrows = PQntuples(res);
  for (int i = 0; i < nrows; i++) {
    char *id = PQgetvalue(res, i, 0);
    char *name = PQgetvalue(res, i, 1);
    char *privileges = PQgetvalue(res, i, 2);
    char *email = PQgetvalue(res, i, 3);
    printf("Id: %s, Name: %s, Privileges: %d, Email: %s, \n", id, name,
           privileges[0], email);
  }

  printf("Total: %d rows\n", nrows);

  clearRes();
  terminate(0);

  return 0;
}

*/