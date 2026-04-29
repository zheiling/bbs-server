/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2026 Oleksandr Zhylin */

#include "../main.h"
#include <db.h>
#include <endian.h>
#include <openssl/sha.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#define UNUSED(x) (void)(x)
#define Q_LEN 128
#define BIN 1
#define TEXT 0
#define ENCR_SIZE 2048

static sqlite3 *db;

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

int32_t db_user_auth(i_auth_t *c, o_auth_t *r) { return 0; }

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