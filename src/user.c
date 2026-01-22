#include "db.h"
#include "main.h"
#include "session.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int32_t create_user(session *sess, char *line) {
  i_db_user_create p;
  uint32_t privileges;
  uint32_t res;
  if (sscanf(line, "register %s %s %s", p.uname, p.pass, p.email) == 3) {
    res = db_user_create(&p);
    if (res) {
      sess->state = OP_WAIT;
      sess->uname = malloc(strlen(p.uname));
      strcpy(sess->uname, p.uname);
      sess->uid = res;
      sess->privileges = 1; // by default
      session_send_string(sess, "ok");
    }
    return res;
  };
  return 0;
}

uint32_t process_user_name(char *line, session *sess) {
  if (!strncmp(line, "exit", sizeof "exit")) {
    sess->state = ERR;
    sess->reason = EXIT;
    session_send_string(sess, "Bye!");
    return 1;
  }

  if (!strncmp(line, "register", sizeof "register" - 1)) {
    return create_user(sess, line);
  }

  if (!strncmp(line, "anonymous", sizeof "anonymous")) {
    sess->uname = malloc(sizeof "anonymous");
    strcpy(sess->uname, "anonymous");
    sess->state = OP_WAIT;
    session_send_string(sess, "Welcome, Anonymous!");
    return 3;
  }

  sess->uname = malloc(strlen(line) + 1);
  strcpy(sess->uname, line);
  sess->state = OP_LOGIN_PSS;
  session_send_string(sess, "password> ");

  return 0;
}

/* returns 1 if success */
int login(session *sess, char *pass) {
  char tmp_string[256];
  i_auth_t cred;
  o_auth_t response;
  cred.name = sess->uname;
  cred.pass = pass;
  response.uid = 0;
  db_user_auth(&cred, &response);
  if (response.uid) {
    sess->state = OP_WAIT;
    sess->privileges = (char)atoi(&response.privileges);
    sess->uid = response.uid;
    sprintf(tmp_string, "Welcome, %s\n", sess->uname);
    session_send_string(sess, tmp_string);
    return 0;
  } else {
    sess->state = ERR;
    sess->reason = EXIT;
    return 1;
  }
}

/* returns 1 if session needs to be closed, 0 - in the opposite case */
int process_error(session *sess) {
  if (sess->state == ERR) {
    switch (sess->reason) {
    case NO_REASON:
    case EXIT:
      return 1;
      break;
    case LOGIN:
      session_send_string(sess, "Can't find the user with such credentials\04\n");
      sess->state = OP_LOGIN_USR;
      sess->reason = NO_REASON;
      session_send_string(sess, "login_again> ");
      break;
    }
  }

  return 0;
}
