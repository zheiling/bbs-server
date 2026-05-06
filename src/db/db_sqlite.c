/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2026 Oleksandr Zhylin */

#include "../main.h"
#include "db_common.h"
#include <db.h>
#include <endian.h>
#include <openssl/sha.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
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

#define SQLITE3_COLUMN_TEXT(stmt, iCol)                                        \
  (const char *)sqlite3_column_text(stmt, iCol)

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
                     "  'id' INTEGER NOT NULL UNIQUE,"
                     "  'username' VARCHAR(50) NOT NULL UNIQUE,"
                     "  'password' CHARACTER(64) NOT NULL,"
                     "  'email' VARCHAR(255) NOT NULL UNIQUE,"
                     "  'created_at' TIMESTAMP NOT NULL,"
                     "  'last_login' TIMESTAMP NULL,"
                     "  'privileges' SMALLINT NOT NULL,"
                     "  CONSTRAINT 'users_pkey' PRIMARY KEY ('id' AUTOINCREMENT),"
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
                     "  'id'INTEGER NOT NULL UNIQUE,"
                     "  'user_id' INTEGER NULL,"
                     "  'name' VARCHAR(255) NOT NULL,"
                     "  'size' BIGINT NOT NULL,"
                     "  'created_at' DATE NOT NULL,"
                     "  'description' TEXT NULL,"
                     "  'permissions' INTEGER NULL,"
                     "  'hash' INTEGER NULL,"
                     "  CONSTRAINT 'files_pkey' PRIMARY KEY('id' AUTOINCREMENT),"
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

// === /* BASE FUNCTIONS */ =========================================

/* ATTENTION: you need to make a copy of responses in callbacks. */
enum db_cb_resp vdb_query(const char *zSql, db_callback callback, void *a_resp,
                          int *count_ptr, enum db_arg_type arg_types[],
                          va_list va_list) {
  sqlite3_stmt *stmt;
  const char *pzTail;
  enum db_cb_resp res = db_no_result;
  size_t size = 0;
  int _count = 0;
  /* Prepare arguments */

  if ((sqlite3_prepare_v2(db, zSql, -1, &stmt, &pzTail)) != SQLITE_OK) {
    fprintf(stderr, "SQL Error: %s\n", sqlite3_errmsg(db));
    return db_err;
  }

  for (int i = 0; arg_types[i] != db_end; i++) {
    switch (arg_types[i]) {
    case db_str:
      sqlite3_bind_text(stmt, i + 1, va_arg(va_list, char *),
                        size > 0 ? size : -1, NULL);
      size = 0;
      break;
    case db_int:
      sqlite3_bind_int(stmt, i + 1, va_arg(va_list, int));
      break;
    case db_int64:
      sqlite3_bind_int64(stmt, i + 1, va_arg(va_list, int64_t));
      break;
    case db_uint:
      sqlite3_bind_int(stmt, i + 1, va_arg(va_list, unsigned int));
      break;
    case db_uint64:
      sqlite3_bind_int64(stmt, i + 1, va_arg(va_list, uint64_t));
      break;
    case db_blob:
      if (size > 0) {
        sqlite3_bind_blob(stmt, i + 1, va_arg(va_list, void *), size, NULL);
        size = 0;
      } else {
        fprintf(stderr, "SQL ERROR: Size for a blob must be specified!\n");
        return db_err;
      }
      break;
    case db_size:
      size = va_arg(va_list, size_t);
      break;
    default:
      break;
    }
  }

  va_end(va_list);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    res = callback(stmt, a_resp);
    if (res == db_err)
      break;
    _count++;
  }

  if (count_ptr != NULL) {
    *count_ptr = _count;
  }

  sqlite3_finalize(stmt);
  return res;
}

enum db_cb_resp db_query(const char *zSql, db_callback callback, void *a_resp,
                         enum db_arg_type arg_types[], ...) {
  va_list args;
  va_start(args, arg_types);
  return vdb_query(zSql, callback, a_resp, NULL, arg_types, args);
}

enum db_cb_resp db_query_count(const char *zSql, db_callback callback,
                               void *a_resp, int *count_ptr,
                               enum db_arg_type arg_types[], ...) {
  va_list args;
  va_start(args, arg_types);
  return vdb_query(zSql, callback, a_resp, count_ptr, arg_types, args);
}

// === /* REALIZATIONS */ ===========================================

// --- /* AUTH USER */ ----------------------------------------------

struct db_user_login_data {
  char *passwordHashed;
  o_auth_t *r;
};

enum db_cb_resp db_user_auth_cb(sqlite3_stmt *stmt, void *resp) {
  struct db_user_login_data *data = (struct db_user_login_data *)resp;
  const char *pass = SQLITE3_COLUMN_TEXT(stmt, 2);
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
                "WHERE username= ?";
  res = db_query(zSql, db_user_auth_cb, (void *)&data, arg_types, c->name);

  if (res == db_success) {
    char u_buf[128];
    sprintf(u_buf, "UPDATE users SET last_login = NOW() WHERE id = %u", r->uid);
    sqlite3_exec(db, u_buf, NULL, NULL, NULL);
    return r->uid;
  }
  return 0;
}

// --- /* CREATE USER */ --------------------------------------------

struct db_user_create_data {
  int uid;
};

enum db_cb_resp db_user_create_cb(sqlite3_stmt *stmt, void *resp) {
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
                " VALUES ($1, $2, $3, 1, date(), date()) RETURNING id";

  res = db_query(zSql, db_user_create_cb, (void *)&data, arg_types, args->uname,
                 passHashed, args->email);

  return data.uid;
}

// --- /* SAVE FILE */ ----------------------------------------------

struct db_save_file_data {
  int id;
};

enum db_cb_resp db_save_file_cb(sqlite3_stmt *stmt, void *resp) {
  struct db_save_file_data *data = (struct db_save_file_data *)resp;
  enum db_cb_resp res = db_no_result;
  data->id = sqlite3_column_int(stmt, 0);
  res = db_success;
  return res;
};

int32_t db_save_file(session *s) {
  s_file_t *sfP = s->file;
  struct db_save_file_data resp_data = {.id = 0};
  const char empty_str[] = "";
  enum db_cb_resp res = db_no_result;
  enum db_arg_type arg_types[] = {db_int, db_str, db_int64, db_int,
                                  db_str, db_int, db_end};

  res = db_query("INSERT INTO files(user_id, name, size, created_at, "
                 "hash, description, permissions) "
                 "VALUES ($1, $2, $3, NOW(), $4, $5, $6) "
                 "RETURNING id",
                 db_save_file_cb, &resp_data, arg_types, s->uid, sfP->name,
                 sfP->size, sfP->hash,
                 sfP->description != NULL ? sfP->description : empty_str,
                 sfP->permissions);

  if (res != db_success) {
    return -1;
  }

  return resp_data.id;
}

// --- /* GET FILE */ -----------------------------------------------

struct db_get_file_data {
  s_file_t **sfPP;
};

enum db_cb_resp db_get_file_db(sqlite3_stmt *stmt, void *resp) {
  struct db_get_file_data *data = (struct db_get_file_data *)resp;
  s_file_t *sf = *(data->sfPP);
  enum db_cb_resp res = db_err;

  sf = malloc(sizeof(s_file_t));
  sf->id = sqlite3_column_int(stmt, 0);
  sf->owner_id = sqlite3_column_int(stmt, 1);
  const char *name = SQLITE3_COLUMN_TEXT(stmt, 2);
  sf->name = malloc(strlen(name) + 1);
  strcpy(sf->name, name);
  sf->size = sqlite3_column_int64(stmt, 3);
  const char *description = SQLITE3_COLUMN_TEXT(stmt, 4);
  sf->description = malloc(strlen(description) + 1);
  strcpy(sf->description, description);
  sf->permissions = sqlite3_column_int(stmt, 5);
  sf->hash = sqlite3_column_int(stmt, 6);
  sf->rest = sf->size;
  *(data->sfPP) = sf;
  res = db_success;

  return res;
}

s_file_t *db_get_file(i_get_file_db *arg) {
  s_file_t *sf = NULL;
  struct db_get_file_data resp_data = {.sfPP = &sf};
  char s_field[24];
  const char *paramValues[1];
  enum db_arg_type arg_types[] = {db_int, db_end};
  char zSql[512];
  int32_t q_len = 0;
  int res = 0;

  if (arg->id) {
    strcpy(s_field, "id");
  } else if (arg->user_id) {
    strcpy(s_field, "user_id");
  } else if (strlen(arg->name)) {
    strcpy(s_field, "name");
  } else {
    fprintf(stderr, "db_get_file: none of the args is specified!\n");
    return NULL;
  }

  sprintf(
      zSql,
      "SELECT files.id, user_id, name, size, description, permissions, hash "
      "FROM files JOIN users ON user_id = users.id WHERE files.%s = $1%n",
      s_field, &q_len);

  if (arg->id) {
    res = db_query(zSql, db_get_file_db, &resp_data, arg_types, arg->id);
  } else if (arg->user_id) {
    res = db_query(zSql, db_get_file_db, &resp_data, arg_types, arg->user_id);
  } else if (strlen(arg->name)) {
    arg_types[0] = db_str;
    res = db_query(zSql, db_get_file_db, &resp_data, arg_types, arg->name);
  }

  if (res != db_success) {
    /* TODO: process error */
  }

  return sf;
}

// --- /* GET FILES LIST */ -----------------------------------------

struct db_get_files_data {
  fl_t **fl_start;
  fl_t **fl_current;
};

enum db_cb_resp db_get_files_data_db(sqlite3_stmt *stmt, void *resp) {
  struct db_get_files_data *data = (struct db_get_files_data *)resp;
  fl_t **fl_start = data->fl_start;
  fl_t *fl_current = *(data->fl_current);
  fl_t *l_item = malloc(sizeof(fl_t));
  l_item->next = NULL;
  l_item->id = sqlite3_column_int(stmt, 0);
  l_item->owner_id = sqlite3_column_int(stmt, 1);
  const char *name = SQLITE3_COLUMN_TEXT(stmt, 2);
  l_item->name = malloc(strlen(name) + 1);
  strcpy(l_item->name, name);
  l_item->size = sqlite3_column_int64(stmt, 3);
  const char *description = SQLITE3_COLUMN_TEXT(stmt, 4);
  l_item->description = malloc(strlen(description) + 1);
  strcpy(l_item->description, description);
  l_item->permissions = sqlite3_column_int(stmt, 5);
  l_item->hash = sqlite3_column_int(stmt, 6);
  const char *owner = SQLITE3_COLUMN_TEXT(stmt, 7);
  l_item->owner = malloc(strlen(owner) + 1);
  strcpy(l_item->owner, owner);

  if (*fl_start == NULL) {
    *fl_start = l_item;
  } else {
    fl_current->next = l_item;
  }
  *(data->fl_current) = l_item;
  return db_success;
}

enum db_cb_resp db_get_files_count_db(sqlite3_stmt *stmt, void *resp) {
  int *_count_full = (int *)resp;
  *_count_full = sqlite3_column_int(stmt, 0);
  return db_success;
}

uint64_t db_get_files_data(i_get_files_db *arg, fl_t **fl_start,
                           uint64_t *full_count) {
  fl_t *fl_current = NULL;
  char zSql[512];
  struct db_get_files_data data = {.fl_current = &fl_current,
                                   .fl_start = fl_start};

  char sort_by[16] = "id";
  char sort_dir[5] = "ASC";

  int count = 0;
  int n_limit = htobe64(arg->limit);
  int n_offset = htobe64(arg->offset);
  enum db_cb_resp res;
  bool by_name = strlen(arg->search_str) > 0;
  int32_t params_num = 3;

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

  if (!by_name) {
    params_num = 2;
    enum db_arg_type arg_types[] = {db_int, db_int, db_end};
    sprintf(zSql,
            "SELECT files.id, user_id, name, size, description, permissions, "
            "hash, username "
            "FROM files JOIN users ON user_id = users.id "
            "ORDER BY files.%s %s LIMIT ? OFFSET ?",
            sort_by, sort_dir);
    res = db_query_count(zSql, db_get_files_data_db, (void *)&data, &count,
                         arg_types, arg->limit, arg->offset);
  } else {
    enum db_arg_type arg_types[] = {db_str, db_int, db_int, db_end};
    sprintf(zSql,
            "SELECT files.id, user_id, name, size, description, permissions, "
            "hash, username "
            "FROM files JOIN users ON user_id = users.id "
            "WHERE name LIKE '%%' || ? || '%%'"
            "ORDER BY files.%s %s LIMIT ? OFFSET ?",
            sort_by, sort_dir);
    res = db_query_count(zSql, db_get_files_data_db, (void *)&data, &count,
                         arg_types, arg->search_str, arg->limit, arg->offset);
  }

  if (res == db_success || res == db_no_result) {
    if (by_name) {
      enum db_arg_type _arg_types[] = {db_int, db_end};

      res = db_query("SELECT COUNT(id) "
                     "FROM files "
                     "WHERE name LIKE '%%' || ? || '%%'",
                     db_get_files_count_db, full_count, _arg_types,
                     arg->search_str);
    } else {
      enum db_arg_type _arg_types[] = {db_end};
      res = db_query("SELECT COUNT(id) "
                     "FROM files ",
                     db_get_files_count_db, full_count, _arg_types);
    }

    if (res == db_success) {
      return count;
    }
  }

  return 0;
}