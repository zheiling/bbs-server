/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2026 Oleksandr Zhylin */

#ifndef USER_H
#define USER_H

#include "main.h"
int login(session *sess, char *pass);
int process_user_name(char *uname, session *sess);
#endif