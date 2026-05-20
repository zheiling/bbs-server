/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2026 Oleksandr Zhylin */

#include "db.h"
#include "file_p.h"
#include "main.h"
#include "session.h"
#include "utils.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

volatile sig_atomic_t shutdown_requested = 0;
volatile sig_atomic_t signal_received = 0;

void term_int_handler(int s) {
  shutdown_requested = 1;
  signal_received = s;
}

void server_main_loop(server_data_t *s_d) {
  fd_set readfds;
  int i, sr;
  sigset_t mask, orig_mask;

  signal(SIGINT, term_int_handler);
  signal(SIGTERM, term_int_handler);
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);
  sigprocmask(SIG_BLOCK, &mask, &orig_mask);
  session *connections[MAX_CONNECTIONS];

  for (i = 0; i < MAX_CONNECTIONS; i++) {
    connections[i] = NULL;
  }

  int maxfd, res;

  for (;;) {
    FD_ZERO(&readfds);
    FD_SET(s_d->ls, &readfds);
    maxfd = s_d->ls;

    for (i = 0; i < MAX_CONNECTIONS; i++) {
      if (connections[i] != NULL) {
        FD_SET(i, &readfds);
        if (i > maxfd)
          maxfd = i;
        /* active uploads/downloads */
        if (connections[i]->fd > -1) {
          int fd = connections[i]->fd;
          FD_SET(fd, &readfds);
          if (fd > maxfd)
            maxfd = fd;
        }
      }
    }

    sr = pselect(maxfd + 1, &readfds, NULL, NULL, NULL, &orig_mask);

    if (shutdown_requested != 0) {
      db_close_connection();
      print_log(stdout, pl_info, "\nStopping the server...\n");
      exit(signal_received);
    }

    if (sr == -1) {
      perror("select");
      exit(6);
    }

    if (FD_ISSET(s_d->ls, &readfds)) {
      if (-1 == accept_client(s_d->ls, connections, s_d->welcome_message)) {
        print_log(stdout, pl_error, "Can't accept connection, error: %d\n",
                  errno);
      }
    }

    for (i = 0; i < MAX_CONNECTIONS; i++) {
      /* download/upload file */
      if (connections[i] != NULL && (connections[i]->state == OP_UPLOAD ||
                                     connections[i]->state == OP_DOWNLOAD)) {
        if (connections[i]->fd > -1 && FD_ISSET(connections[i]->fd, &readfds)) {
          perform_session_action(connections[i], NULL, s_d);
        }
      } else if (connections[i] != NULL && FD_ISSET(i, &readfds)) {
        /* server/client requests */
        session *sess = connections[i];
        char *line;
        res = session_do_read(sess, &line);
        if (line != NULL && res) {
          perform_session_action(sess, line, s_d);
          free(line);
          if (sess->state == ERR) {
            close_session(connections, i);
          }
        }
        if (res == 0) {
          close_session(connections, i);
        }
      }
    }
  }
}

char *get_welcome_mes(void) {
  int fd = open(WELCOME_FILE_NAME, O_RDONLY);
  if (fd == -1) {
    perror(WELCOME_FILE_NAME);
    exit(7);
  }
  int filesize = lseek(fd, 0, SEEK_END) + 1;
  lseek(fd, 0, SEEK_SET);
  char *welcome_message =
      mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  if (welcome_message == MAP_FAILED) {
    perror(WELCOME_FILE_NAME);
    close(fd);
    exit(5);
  }
  welcome_message[filesize - 1] = '\04';
  close(fd);
  return welcome_message;
}

int start_server(void) {
  int res;
  int ls = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in s_addr;

  s_addr.sin_family = AF_INET;
  s_addr.sin_port = htons(PORT);
  s_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  int opt = 1;
  setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  res = bind(ls, (struct sockaddr *)&s_addr, sizeof(s_addr));
  if (res == -1) {
    perror("bind");
    exit(3);
  }

  res = listen(ls, LISTEN_QLEN);
  if (res == -1) {
    perror("listen");
    exit(4);
  }
  return ls;
}

void prepare_start(int argc, char *argv[]) {
  if (argc != 2) {
    print_log(stdout, pl_error, "Usage: %s <work_dir>\n", argv[0]);
    exit(1);
  }

  if (-1 == chdir(argv[1])) {
    perror(argv[1]);
    exit(2);
  }

  if (!directory_exists(STORAGE_FOLDER)) {
    mkdir(STORAGE_FOLDER, 0700);
  }
}