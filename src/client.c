#include "client.h"
#include "file_p.h"
#include "main.h"
#include "session.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define DEF_LIMIT 15
#define DEF_PAGE_NUM 1

void process_client_command(char *line, session *sess, server_data_t *s_d) {
  char arg_1[32];
  char arg_2[32];
  uint32_t res;
  i_file_list_t fl_args;
  fl_args.name = NULL;
  int32_t ret = 0;

  sscanf(line, "%s %s", arg_1, arg_2);

  if (!strcmp(arg_1, "file")) {
    /* LIST */
    if (!strcmp(arg_2, "list")) {
      ret = sscanf(line, "%*s %*s %u %u", &(fl_args.limit), &(fl_args.page));
      switch (ret) {
      case EOF:
      case 0:
        fl_args.limit = DEF_LIMIT;
      case 1:
        fl_args.page = DEF_PAGE_NUM;
      }
      file_list(sess, s_d, &fl_args);
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

    /* SEARCH */
    if (!strcmp(arg_2, "search")) {
      char *s_type = arg_1; /* since we don't need the contents of arg_1 and
                               arg_2 variables */
      char *s_val = arg_2;
      ret = sscanf(line, "%*s %*s %s %s %u %u", s_type, s_val, &(fl_args.limit),
                   &(fl_args.page));

      switch (ret) {
      case 2:
        fl_args.limit = DEF_LIMIT;
      case 3:
        fl_args.page = DEF_PAGE_NUM;
      }

      /* by name */
      if (!strcmp(s_type, "name")) {
        fl_args.name = s_val;
      }

      file_list(sess, s_d, &fl_args);
    }
  }

  if (!strncmp(line, "exit", sizeof "exit")) {
    sess->state = ERR;
    sess->reason = EXIT;
    session_send_string(sess, "OK. Bye!\n");
  }
}