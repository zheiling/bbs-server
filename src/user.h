#ifndef USER_H
#define USER_H

#include "main.h"
int login(session *sess, char *pass);
int process_error(session *sess);
long int find_user(char *uname);
#endif