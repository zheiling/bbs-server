/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2026 Oleksandr Zhylin */

#include "main.h"
#include "server.h"
#include "db.h"
#include <stdio.h>

/* *** MAIN *** */
int main(int argc, char *argv[]) {
  server_data_t server_data;
  prepare_start(argc, argv);
  server_data.welcome_message = get_welcome_mes();
  init_db_connection();
  server_data.ls = start_server();
  /* TODO: display server port */
  printf("Server is started!\n");
  server_main_loop(&server_data);
  return 0;
}