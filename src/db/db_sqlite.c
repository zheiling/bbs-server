/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2026 Oleksandr Zhylin */

#include "../main.h"
#include <db.h>
#include <endian.h>
#include <openssl/sha.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

typedef enum db_cb_resp (*db_callback)(sqlite3_stmt *stmt, void **resp);

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
  int res = sqlite3_open("db.sql", &db);
  char *err = NULL;
  if (!res) {
    res = check_and_create_tables();
  }

  return 0;
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

enum db_cb_resp db_query(const char *zSql, db_callback callback, void **a_resp,
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

enum db_cb_resp db_user_auth_db(sqlite3_stmt *stmt, void **resp) {
  return db_success;
  /* TODO: complete */
}

int32_t db_user_auth(i_auth_t *c, o_auth_t *r) {
  enum db_arg_type args[] = {db_str};
  long int user_id;
  char zSql[] = "SELECT id, username, password, privileges "
                "FROM users "
                "WHERE username= $1";
  db_query(zSql, db_user_auth_db, (void *)&user_id, args, c->name);

  /* TODO: complete */
  return 0;
}

int32_t db_user_create(i_db_user_create *args) { return 0; }

int32_t db_save_file(session *s) { return 0; }

// TODO: get file
s_file_t *db_get_file(i_get_file_db *arg) {
  s_file_t *sf = NULL;

  return sf;
}

uint64_t db_get_files_data(i_get_files_db *arg, fl_t **fl_start,
                           uint64_t *full_count) {
  fl_t *fl_current = NULL;
  return 0;
}