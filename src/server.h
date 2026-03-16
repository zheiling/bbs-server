/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2026 Oleksandr Zhylin */

#ifndef SERVER_H
#define SERVER_H
#include "main.h"
int start_server(void);
char *get_welcome_mes(void);
void server_main_loop(server_data_t *);
void prepare_start(int argc, char *argv[]);
#endif