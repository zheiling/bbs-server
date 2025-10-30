#include "main.h"
#include "session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "db.h"

int process_user_name(char *uname, session *sess) {
  if (!strncmp(uname, "exit", sizeof "exit")) {
    sess->state = ERR;
    sess->reason = EXIT;
    session_send_string(sess, "Bye!");
    return 1;
  }

  if (!strncmp(uname, "register", sizeof "register")) {
    sess->state = OP_REGISTER;
    session_send_string(sess, "Your username > ");
    return 2;
  }

  if (!strncmp(uname, "anon", sizeof "anon")) {
    sess->state = OP_LOGIN_ANON;
    session_send_string(sess, "Welcome, Anonymous!");
    return 3;
  }

  sess->uname = malloc(strlen(uname) + 1);
  strcpy(sess->uname, uname);
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
