#include "main.h"
#include "file_p.h"
#include "server.h"

/* *** MAIN *** */
int main(int argc, char *argv[]) {
  server_data_t server_data;
  prepare_start(argc, argv);
  server_data.welcome_message = get_welcome_mes();
  server_data.fl_start = NULL;
  server_data.fl_current = NULL;
  get_files_descriptions(&server_data);
  server_data.ls = start_server();
  server_main_loop(&server_data);
  return 0;
}