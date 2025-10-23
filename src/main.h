#ifndef MAIN_H
#define MAIN_H
#include <stddef.h>
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

typedef struct session {
  unsigned long from_ip;
  unsigned short from_port;
  char buf[INBUFSIZE];
  int buf_used;
  enum sess_states state;
  enum error_reason reason;
  char privileges;
  char *uname;
  int sd; /* session descriptor */
  int fd; /* file descriptor */
  char *fname;
  char *fpath;
  char f_perm;
  char *fdesc;
  size_t fsize;
  size_t f_rest;
  long int userpos; /* hint to the position in user list */
} session;

typedef struct fl_t {
  char *name;
  char *description;
  char *owner;
  char permissions;
  size_t size;
  struct fl_t *next;
} fl_t;

typedef struct server_data_t {
  fl_t *fl_start;
  fl_t *fl_current;
  char *welcome_message;
  int ls;
} server_data_t;

#endif