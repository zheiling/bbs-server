#include "session.h"
#include "client.h"
#include "file_p.h"
#include "main.h"
#include "user.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stddef.h>
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
  sess->f_perm = 0;
  sess->fdesc = NULL;
  sess->sd = fd;
  sess->fd = -1;
  sess->fsize = 0;
  sess->f_rest = 0;
  sess->fname = NULL;
  sess->fpath = NULL;
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
      file_save_db(sess, s_d);
      session_send_string(sess, "File is saved!\n");
      sess->state = OP_WAIT;
    }
    break;
  }
  /* } */
}

int file_save_db(session *sess, server_data_t *s_d) {
  /* crate and fill the new item */
  fl_t *nitem = malloc(sizeof(fl_t));
  nitem->description = sess->fdesc;
  nitem->name = sess->fname;
  nitem->size = sess->fsize;
  nitem->owner = malloc(strlen(sess->uname));
  strcpy(nitem->owner, sess->uname);
  nitem->permissions = sess->f_perm;
  nitem->next = NULL;

  /* set to NULL and clear */
  sess->fname = NULL;
  free(sess->fpath);
  sess->fpath = NULL;
  sess->fdesc = NULL;
  if (sess->fd > -1) {
    close(sess->fd);
  }
  sess->fd = -1;
  sess->fsize = 0;
  sess->f_rest = 0;

  /* add new item to list */
  s_d->fl_current->next = nitem;
  s_d->fl_current = nitem;

  /* save to the db file */
  int fd = open(FILE_DESCRIPTIONS_NAME, O_WRONLY);
  lseek(fd, 0, SEEK_END);
  char permissions[4];
  snprintf(permissions, 3, "%d", nitem->permissions);
  unsigned int res_len = strlen(nitem->name) + 1 + strlen(nitem->owner) + 1 +
                         strlen(permissions) + 1 + strlen(nitem->description) +
                         1;
  char *res_str = malloc(res_len);
  res_len = sprintf(res_str, "%s\n%s\n%s\n%s", nitem->name, nitem->owner,
                    permissions, nitem->description);
  nitem->description[strlen(nitem->description) - 6] = 0;
  write(fd, res_str, res_len);
  close(fd);
  return 0;
}

void close_session(session *connections[], int sd) {
  if (connections[sd] != NULL) {
    close(sd);
    connections[sd]->sd = -1;
    free(connections[sd]->uname);
    free(connections[sd]);
    connections[sd] = NULL;
  }
}