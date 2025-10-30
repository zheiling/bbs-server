#include "session.h"
#include "client.h"
#include "file_p.h"
#include "main.h"
#include "user.h"
#include "db.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

session *make_new_session(int fd, struct sockaddr_in *from, char *wm);

int accept_client(int ls, session *connections[], char *wm) {
  int sd;
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  sd = accept(ls, (struct sockaddr *)&addr, &len);
  if (sd == -1) {
    perror("accept");
    return -1;
  }
  session *sess = make_new_session(sd, &addr, wm);
  fprintf(stdout, "New connection: %s:%u\n", inet_ntoa(addr.sin_addr),
          ntohs(addr.sin_port));
  connections[sd] = sess;
  return 0;
}

session *make_new_session(int fd, struct sockaddr_in *from, char *wm) {
  struct session *sess = malloc(sizeof(*sess));
  sess->from_ip = ntohl(from->sin_addr.s_addr);
  sess->from_port = ntohs(from->sin_port);
  sess->buf_used = 0;
  sess->state = OP_LOGIN_USR;
  sess->reason = NO_REASON;
  sess->uname = NULL;
  sess->sd = fd;
  sess->file = NULL;
  session_send_string(sess, wm);
  session_send_string(sess, "login> ");
  return sess;
}

void session_send_string(struct session *sess, const char *str) {
  write(sess->sd, str, strlen(str));
}

int session_do_read(session *sess, char **read_str) {
  int rc, bufp = sess->buf_used;
  rc = read(sess->sd, sess->buf + bufp, INBUFSIZE - bufp);
  sess->buf[bufp + rc] = 0;
  if (rc <= 0) {
    sess->state = ERR;
    return 0;
  }
  sess->buf_used += rc;
  char *line;
  query_extract_from_buf(sess, &line);
  if (line == NULL) {
    *read_str = line;
    return 0;
  } else {
    *read_str = line;
  }
  if (sess->buf_used >= INBUFSIZE) {
    /* we can't read further, no room in the buffer, no whole line yet */
    session_send_string(sess, "Line too long! Good bye...\n");
    sess->state = ERR;
    return 0;
  }
  if (sess->state == FIN)
    return 0;
  return 1;
}

// TODO: синхронизовать c C++
int query_extract_from_buf(session *sess, char **output_line) {
  char *line;
  int pos = -1;

  if (sess->buf_used > 0) {
    char *cptr = strchr(sess->buf, '\n');
    if (cptr != NULL)
      pos = cptr - sess->buf;
  } else {
    return 0;
  }

  if (pos == -1) {
    int b_used = sess->buf_used;
    sess->buf_used = 0;
    line = malloc(b_used + 1);
    strncpy(line, sess->buf, b_used);

    line[b_used] = 0;

    *output_line = line;
    return b_used;
  } else {
    line = malloc(pos + 2);
    strncpy(line, sess->buf, pos + 1);
    line[++pos] = 0;
    sess->buf_used -= (pos);
    if (!pos)
      pos++;
    memmove(sess->buf, sess->buf + pos, sess->buf_used);
    sess->buf[sess->buf_used] = 0;
    if (line[pos-2] == '\r') {
      line[pos-2] = line[pos-1];
      pos--;
      line[pos-1] = 0;
    }
    *output_line = line;
  }
  return pos + 1;
}

void perform_session_action(session *sess, char *line, server_data_t *s_d) {
  int state = sess->state;
  int priv;
  int res;
  /* while (sess->buf_used) { */
  switch (state) {
  case OP_LOGIN_USR:
    process_user_name(line, sess);
    break;
  case OP_LOGIN_PSS:
    login(sess, line);
    break;
  case OP_WAIT:
    process_client_command(line, sess, s_d);
    break;
  case OP_DOWNLOAD:
    file_download_upload(sess, F_DOWNLOAD);
    break;
  case OP_UPLOAD:
    file_download_upload(sess, F_UPLOAD);
    break;
  case OP_UPLOAD_DESCRIPTION:
    res = file_upload_description(sess, line, s_d);
    if (res) {
      if (db_save_file(sess)) {
        clear_file_from_sess(sess);
        session_send_string(sess, "File is saved!\n");
        sess->state = OP_WAIT;
      } else {
        // TODO: error case
      }
    }
    break;
  }
  /* } */
}

void close_session(session *connections[], int sd) {
  if (connections[sd] != NULL) {
    if (connections[sd]->file) clear_file_from_sess(connections[sd]);
    close(sd);
    connections[sd]->sd = -1;
    free(connections[sd]->uname);
    free(connections[sd]);
    connections[sd] = NULL;
  }
}