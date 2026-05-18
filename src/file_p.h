/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2026 Oleksandr Zhylin */

#ifndef FILEP_H
#define FILEP_H
#include "main.h"
#include <stdint.h>

typedef struct {
    int32_t page;
    int32_t limit;
    char *name;
} i_file_list_t;

enum f_actions {
    F_UPLOAD,
    F_DOWNLOAD,
};

void clear_list(fl_t *start);
void file_list(session *, server_data_t *, i_file_list_t *l_args);
int  file_send_prepare(session *, char *, server_data_t *);
int  file_receive_prepare(session *sess, char *line, server_data_t *s_d);
int  file_upload_description(session *sess, char *line, server_data_t *s_d);
int  file_save_db(session *sess, server_data_t *s_d);
void clear_file_from_sess(session *);
int  directory_exists(const char *path);
void file_download(session *sess);
void file_upload(session *sess);
#endif