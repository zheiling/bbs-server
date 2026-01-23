#include "client.h"
#include "file_p.h"
#include "main.h"
#include "session.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void process_client_command(char *line, session *sess, server_data_t *s_d) {
  char arg_1[32];
  char arg_2[32];
  uint32_t res;
  uint32_t limit, page;

  sscanf(line, "%s %s", arg_1, arg_2);

  if (!strcmp(arg_1, "file")) {
    /* LIST */
    if (!strcmp(arg_2, "list")) {
      sscanf(line, "%*s %*s %u %u", &limit, &page);
      file_list(sess, s_d, limit, page);
      return;
    }

    /* UPLOAD */
    if (!strcmp(arg_2, "upload")) {
      res = file_receive_prepare(sess, line, s_d);
      if (!res) 
        sess->state = OP_UPLOAD;
      return;
    }

    /* DOWNLOAD */
    if (!strcmp(arg_2, "download")) {
      res = file_send_prepare(sess, line, s_d);
      if (!res)
        sess->state = OP_DOWNLOAD;
      return;
    }
  }

  if (!strncmp(line, "exit", sizeof "exit")) {
    sess->state = ERR;
    sess->reason = EXIT;
    session_send_string(sess, "OK. Bye!\n");
  }
}