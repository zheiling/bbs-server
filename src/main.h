#ifndef MAIN_H
#define MAIN_H
#include <stddef.h>
#include <stdint.h>
#define PORT 2000
#define MAX_CONNECTIONS 1024
#define INBUFSIZE 1024
#define FILEBUFSIZE 4096
#define WSIZE 128
#define LISTEN_QLEN 32
#define STORAGE_FOLDER "./storage"
#define FILE_DESCRIPTIONS_NAME "./file_descriptions"
#define FILE_USER_LIST_NAME "userlist"
#define WELCOME_FILE_NAME "welcome"

enum sess_states {
  OP_EDIT_FILE,
  OP_DOWNLOAD,
  OP_EDIT_USER,
  OP_LOGIN_USR,
  OP_LOGIN_PSS,
  OP_LOGIN_ANON,
  OP_WAIT,
  OP_UPLOAD,
  OP_UPLOAD_DESCRIPTION,
  OP_REGISTER,
  ERR,
  FIN
};

enum user_privileges {
  ADD_FILE = 01,
  EDIT_FILES = 02,
  REMOVE_FILES = 04,
  EDIT_USER = 010
};

enum error_reason {
  NO_REASON,
  LOGIN,
  EXIT,
};

typedef struct {
  char *name;
  char *path;
  char *description;
  char permissions;
  size_t size;
  size_t rest;
  uint32_t hash;
} session_file;

typedef struct fl_t {
  char *name;
  char *description;
  char *owner;
  char permissions;
  uint32_t owner_id;
  uint32_t hash;
  size_t size;
  struct fl_t *next;
} fl_t;

typedef struct session {
  unsigned long from_ip;
  unsigned short from_port;
  char buf[INBUFSIZE];
  int buf_used;
  enum sess_states state;
  enum error_reason reason;
  char privileges;
  char *uname;
  uint32_t uid;
  int fd;
  int sd; /* session descriptor */
  session_file *file;
  fl_t *fl_start;
  fl_t *fl_current;
} session;

typedef struct server_data_t {
  char *welcome_message;
  int ls;
} server_data_t;

#define FILE_HASH_SEED 898

#endif