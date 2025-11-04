#include "main.h"
#include "server.h"
#include "db.h"

/* *** MAIN *** */
int main(int argc, char *argv[]) {
  server_data_t server_data;
  prepare_start(argc, argv);
  server_data.welcome_message = get_welcome_mes();
  init_db_connection();
  server_data.ls = start_server();
  server_main_loop(&server_data);
  return 0;
}