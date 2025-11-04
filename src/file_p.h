#ifndef FILEP_H
#define FILEP_H
#include "main.h"
void get_files_data(server_data_t *, uint32_t page);
void clear_list(fl_t *start);
void file_list(session *, server_data_t *);
int file_send_prepare(session *, char *, server_data_t *);
enum f_actions {
    F_UPLOAD,
    F_DOWNLOAD,
};
void file_load(session *sess, enum f_actions);
int file_receive_prepare(session *sess, char *line, server_data_t *s_d);
int file_upload_description(session *sess, char *line, server_data_t *s_d);
int file_save_db(session *sess, server_data_t *s_d);
void clear_file_from_sess(session *);
#endif