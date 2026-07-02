/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2026 Oleksandr Zhylin */

#include <client.h>
#include <fcntl.h>
#include <file_p.h>
#include <main.h>
#include <session.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <utils.h>

void process_client_command(char *line, session *sess, server_data_t *s_d) {
  char arg_1[32];
  char arg_2[32];
  int32_t res;
  i_file_list_t fl_args;
  fl_args.name = NULL;
  int32_t ret = 0;

  ret = sscanf(line, "%s %s", arg_1, arg_2);

  if (ret == 1) {
    if (!strcmp(arg_1, "file")) {
      session_send_string(sess,
                          "Available commands: list, upload, download, search");
      return;
    }
  }

  if (!strcmp(arg_1, "file")) {
    /* LIST */
    if (!strcmp(arg_2, "list")) {
      ret = sscanf(line, "%*s %*s %u %u", &(fl_args.limit), &(fl_args.page));
      switch (ret) {
      case EOF:
      case 0:
        fl_args.limit = DIF_LIMIT;
      case 1:
        fl_args.page = DIF_PAGE_NUM;
      }
      file_list(sess, &fl_args);
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
      char *s_val = NULL;
      ret = sscanf(line, "%*s %*s %s %s %u %u", s_type, s_val, &(fl_args.limit),
                   &(fl_args.page));

      switch (ret) {
      case 0:
      case -1:
        session_send_string(sess, "The search criteria is not specified!");
        return;
      case 1:
        session_send_string(sess, "The file name is not specified!");
        return;
      case 2:
        fl_args.limit = DIF_LIMIT;
      case 3:
        fl_args.page = DIF_PAGE_NUM;
      }

      /* by name */
      if (!strcmp(s_type, "name")) {
        fl_args.name = s_val;
      }

      file_list(sess, &fl_args);
    }
  }

  if (!strncmp(line, "exit", sizeof "exit")) {
    sess->state = ERR;
    sess->reason = EXIT;
    session_send_string(sess, "OK. Bye!\n");
  }
}

void download_confirm(char *line, session *sess, server_data_t *s_d) {
  if (!strncmp(line, "continue", sizeof "continue" - 1)) {
    int pac_siz = PACKAGE_SIZE;
    sscanf(line, "continue %d\n", &pac_siz);
    sess->state = OP_DOWNLOAD;
    sess->file->package_rest = pac_siz;
    return;
  }
  if (!strncmp(line, "cancel", sizeof "cancel" - 1)) {
    print_log(stdout, pl_info, "Downloading of %s is canceled by the client\n",
              sess->file->name);
    clear_file_from_sess(sess);
    sess->state = OP_WAIT;
    return;
  }
  print_log(stdout, pl_info, "The answer is not correct. Send either \"continue \%PACKAGE_SIZE\% or \"cancel\"");
}