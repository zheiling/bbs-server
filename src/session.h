#ifndef SESSION_H
#define SESSION_H
#include "main.h"
int accept_client(int ls, session *connections[], char *wm);
void session_send_string(struct session *sess, const char *str);
int query_extract_from_buf(session *sess, char **output_line);
int session_do_read(session *sess, char **read_str);
void perform_session_action(session *sess, char *line, server_data_t *s_d);
void close_session(session *connections[], int sd);
/* session *make_new_session(int fd, struct sockaddr_in *from, char *wm); */
/* int query_extract_from_sess(session *sess, char **output_line); */
#endif