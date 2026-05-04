/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2026 Oleksandr Zhylin */

#include "../main.h"
#include "db_common.h"
#include <db.h>
#include <endian.h>
#include <openssl/sha.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define UNUSED(x) (void)(x)
#define Q_LEN 128
#define BIN 1
#define TEXT 0
#define ENCR_SIZE 2048

static sqlite3 *db;

enum db_arg_type {
  db_str,
  db_int,
  db_int64,
  db_uint,
  db_uint64,
  db_blob,
  db_size, /* size for subsequent arg */
  db_end
};

typedef enum db_cb_resp (*db_callback)(sqlite3_stmt *stmt, void *resp);

void print_err(char **err) {
  if (*err != NULL) {
    printf("%s\n", *err);
    sqlite3_free(*err);
    *err = NULL;
  }
}

int check_and_create_tables() {
  int res = 0;
  char *err = NULL;

  res = sqlite3_exec(db,
                     "CREATE TABLE IF NOT EXISTS 'users' ("
                     "  'id' SERIAL,"
                     "  'username' VARCHAR(50) NOT NULL,"
                     "  'password' CHARACTER(64) NOT NULL,"
                     "  'email' VARCHAR(255) NOT NULL,"
                     "  'created_at' TIMESTAMP NOT NULL,"
                     "  'last_login' TIMESTAMP NULL,"
                     "  'privileges' SMALLINT NOT NULL,"
                     "  CONSTRAINT 'users_pkey' PRIMARY KEY ('id'),"
                     "  CONSTRAINT 'users_email_key' UNIQUE ('email'),"
                     "  CONSTRAINT 'users_username_key' UNIQUE ('username')"
                     "); ",
                     NULL, NULL, &err);

  print_err(&err);
  res = sqlite3_exec(db,
                     "CREATE UNIQUE INDEX IF NOT EXISTS 'username_idx'"
                     " ON 'users' ("
                     " 'username' ASC"
                     " );",
                     NULL, NULL, &err);
  print_err(&err);
  res = sqlite3_exec(db,
                     "CREATE UNIQUE INDEX IF NOT EXISTS 'email_idx' "
                     "ON 'users' ("
                     "'email' ASC"
                     ");",
                     NULL, NULL, &err);
  print_err(&err);
  res = sqlite3_exec(db,
                     "CREATE TABLE IF NOT EXISTS 'files' ("
                     "  'id' SERIAL,"
                     "  'user_id' INTEGER NULL,"
                     "  'name' VARCHAR(255) NOT NULL,"
                     "  'size' BIGINT NOT NULL,"
                     "  'created_at' DATE NOT NULL,"
                     "  'description' TEXT NULL,"
                     "  'permissions' INTEGER NULL,"
                     "  'hash' INTEGER NULL,"
                     "  CONSTRAINT 'files_pkey' PRIMARY KEY ('id'),"
                     "  CONSTRAINT 'fk_user' FOREIGN KEY ('user_id')"
                     "  REFERENCES 'users' ('id') ON DELETE NO ACTION"
                     "  ON UPDATE NO ACTION"
                     ");",
                     NULL, NULL, &err);
  print_err(&err);
  return res;
}

int init_db_connection(void) {
  int res = sqlite3_open("db.sqlite", &db);
  char *err = NULL;
  if (!res) {
    res = check_and_create_tables();
  }

  return 0;
}
/* ATTENTION: you need to make a copy of responses in callbacks. */
enum db_cb_resp db_query(const char *zSql, db_callback callback, void *a_resp,
                         enum db_arg_type arg_types[], ...) {
  sqlite3_stmt *stmt;
  const char *pzTail;
  enum db_cb_resp res = db_no_result;
  size_t size = 0;

  /* Prepare arguments */

  va_list args;
  va_start(args, arg_types);

  if (sqlite3_prepare_v2(db, zSql, -1, &stmt, &pzTail) != SQLITE_OK) {
    return db_err;
  }

  for (int i = 0; arg_types[i] != db_end; i++) {
    switch (arg_types[i]) {
    case db_str:
      sqlite3_bind_text(stmt, i + 1, va_arg(args, char *), size > 0 ? size : -1,
                        NULL);
      size = 0;
      break;
    case db_int:
      sqlite3_bind_int(stmt, i + 1, va_arg(args, int));
      break;
    case db_int64:
      sqlite3_bind_int64(stmt, i + 1, va_arg(args, int64_t));
      break;
    case db_uint:
      sqlite3_bind_int(stmt, i + 1, va_arg(args, unsigned int));
      break;
    case db_uint64:
      sqlite3_bind_int64(stmt, i + 1, va_arg(args, uint64_t));
      break;
    case db_blob:
      if (size > 0) {
        sqlite3_bind_blob(stmt, i + 1, va_arg(args, void *), size, NULL);
        size = 0;
      } else {
        fprintf(stderr, "SQL ERROR: Size for a blob must be specified!\n");
        return db_err;
      }
      break;
    case db_size:
      size = va_arg(args, size_t);
      break;
    default:
      break;
    }
  }

  va_end(args);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    res = callback(stmt, a_resp);
    if (res != db_no_result)
      break;
  }

  sqlite3_finalize(stmt);
  return res;
}

struct db_user_login_data {
  char *passwordHashed;
  o_auth_t *r;
};

enum db_cb_resp db_user_auth_db(sqlite3_stmt *stmt, void *resp) {
  struct db_user_login_data *data = (struct db_user_login_data *)resp;
  const unsigned char *pass = sqlite3_column_text(stmt, 2);
  if (!strcmp(data->passwordHashed, (const char *)pass)) {
    data->r->uid = sqlite3_column_int(stmt, 0);
    data->r->privileges = sqlite3_column_int(stmt, 6);
    return db_success;
  } else {
    return db_fail;
  }
}

int32_t db_user_auth(i_auth_t *c, o_auth_t *r) {
  char passHashed[SHA256_DIGEST_LENGTH * 2];
  struct db_user_login_data data = {.r = r};
  string_to_SHA256(c->pass, passHashed);
  data.passwordHashed = passHashed;
  enum db_arg_type arg_types[] = {db_str, db_end};
  long int user_id;
  enum db_cb_resp res;
  char zSql[] = "SELECT id, username, password, privileges "
                "FROM users "
                "WHERE username= $1";
  res = db_query(zSql, db_user_auth_db, (void *)&data, arg_types, c->name);

  if (res == db_success) {
    char u_buf[128];
    sprintf(u_buf, "UPDATE users SET last_login = NOW() WHERE id = %u", r->uid);
    sqlite3_exec(db, u_buf, NULL, NULL, NULL);
    return r->uid;
  }
  return 0;
}

struct db_user_create_data {
  int uid;
};

enum db_cb_resp db_user_create_db(sqlite3_stmt *stmt, void *resp) {
  struct db_user_create_data *data = (struct db_user_create_data *)resp;
  data->uid = sqlite3_column_int(stmt, 0);

  return db_success;
}

int32_t db_user_create(i_db_user_create *args) {
  char passHashed[SHA256_DIGEST_LENGTH * 2];
  struct db_user_create_data data = {.uid = 0};
  string_to_SHA256(args->pass, passHashed);
  enum db_arg_type arg_types[] = {db_str, db_str, db_str, db_end};
  long int user_id;
  enum db_cb_resp res;
  char zSql[] = "INSERT INTO users (username, password, email, "
                "privileges, created_at, last_login)"
                " VALUES ($1, $2, $3, 1, NOW(), NOW()) RETURNING id";

  res = db_query(zSql, db_user_create_db, (void *)NULL, arg_types, args->uname,
                 args->pass, args->email);

  return data.uid;
}

int32_t db_save_file(session *s) { return 0; }

s_file_t *db_get_file(i_get_file_db *arg) {
  s_file_t *sf = NULL;

  return sf;
}

uint64_t db_get_files_data(i_get_files_db *arg, fl_t **fl_start,
                           uint64_t *full_count) {
  fl_t *fl_current = NULL;
  return 0;
}