#include "main.h"
#include "session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

long int find_user(char *uname) {
  FILE *fptr = fopen(FILE_USER_LIST_NAME, "r");

  if (fptr == NULL) {
    perror(FILE_USER_LIST_NAME);
    exit(9);
  }

  char *line = NULL;
  size_t bsize = WSIZE;
  char b_uname[32];
  long int cur_pos = 0;
  int lsize = 0;

  if (!strncmp(uname, "exit", sizeof "exit")) {
    return -2;
  }

  while (getline(&line, &bsize, fptr)) {
    if (line[0] == '\b')
      break;
    sscanf(line, "%s", b_uname);
    if (!strcmp(uname, b_uname)) {
      line = NULL;
      fclose(fptr);
      return cur_pos;
    } else {
      cur_pos += lsize;
      line = NULL;
    }
  }

  fclose(fptr);
  return -1;
}

/* returns 1 if success */
int login(session *sess, char *pass) {
  FILE *fptr = fopen(FILE_USER_LIST_NAME, "r");
  fseek(fptr, sess->userpos, SEEK_SET);
  int res = 0;

  char *line = NULL;
  size_t lsize = WSIZE;
  char *pass_tok, *name_tok, *priv_tok;

  getline(&line, &lsize, fptr);
  name_tok = strtok(line, " ");
  pass_tok = strtok(NULL, " ");

  if (!strcmp(pass_tok, pass)) {
    int n_len = pass_tok - name_tok - 1; /* exclude space */
    sess->uname = malloc(n_len + 1);
    strncpy(sess->uname, name_tok, n_len);
    sess->uname[n_len] = 0;
    priv_tok = strtok(NULL, " ");
    sess->privileges = (char)atoi(priv_tok);
    res = 1;
  }
  fclose(fptr);
  return res;
}

// TODO: синхронизовать c C++
/* returns 1 if session needs to be closed, 0 - in the opposite case */
int process_error(session *sess) {
  if (sess->state == ERR) {
    switch (sess->reason) {
    case NO_REASON:
    case EXIT:
      return 1;
      break;
    case LOGIN:
      session_send_string(sess, "Can't find the user with such credentials\n");
      sess->state = OP_LOGIN_USR;
      sess->reason = NO_REASON;
      session_send_string(sess, "login_again> ");
      break;
    }
  }

  return 0;
}
