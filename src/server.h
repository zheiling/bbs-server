#ifndef SERVER_H
#define SERVER_H
#include "main.h"
int start_server();
char *get_welcome_mes();
void server_main_loop(server_data_t *);
void prepare_start(int argc, char *argv[]);
#endif