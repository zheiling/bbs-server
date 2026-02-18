#include "file_p.h"
#include "db.h"
#include "main.h"
#include "session.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <murmur3.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

char *extract_second_arg(char *);
size_t get_file_size(char *, char *);

void clear_list(fl_t *start) {
  if (start) {
    fl_t *curr = start;
    fl_t *next;
    do {
      next = curr->next;
      free(curr->description);
      free(curr->name);
      free(curr->owner);
      free(curr);
    } while ((curr = next) != NULL);
  }
}

void file_list(session *sess, server_data_t *s_d, i_file_list_t *f_args) {
  fl_t *fl_start, *fl_current;
  char item_h[256];
  i_get_files_db args;
  uint64_t full_count, count, pages_count;

  fl_start = NULL;
  fl_current = NULL;

  if (sess->fl_start) {
    clear_list(fl_start);
    sess->fl_start = NULL;
    sess->fl_current = NULL;
  }

  args.limit = f_args->limit;
  args.offset = f_args->limit * (f_args->page - 1);
  args.sort_by = ID;
  args.sort_direction = ASC;
  if (f_args->name != NULL) {
    strcpy(args.name, f_args->name);
  } else {
    args.name[0] = '\0';
  }

  count = db_get_files_data(&args, &fl_start, &full_count);
  char page_info[256];

  pages_count = full_count / f_args->limit;
  if (!pages_count)
    pages_count = 1;
  if ((full_count > f_args->limit) && (pages_count % f_args->limit))
    pages_count++;

  sprintf(page_info, ":END: PAGE %u/%lu COUNT: %lu/%lu\n", f_args->page,
          pages_count, count, full_count - (f_args->page - 1) * f_args->limit);

  if (!fl_start) {
    write(sess->sd, page_info, strlen(page_info));
    return;
  }

  fl_current = fl_start;

  do {
    int n_len = strlen(sess->uname);
    if (strncmp(fl_current->owner, sess->uname, n_len) &&
        !fl_current->permissions) {
      continue;
    }
    int h_len = sprintf(item_h, "%s %zu %s", fl_current->name, fl_current->size,
                        fl_current->owner);
    int d_len = strlen(fl_current->description);
    int f_len = h_len + d_len + 2;
    char *full_str = malloc(sizeof(char) * f_len);
    sprintf(full_str, "%s %s", item_h, fl_current->description);
    int i;
    for (i = h_len; i < h_len + d_len; i++) {
      if (full_str[i] == '\n') {
        full_str[i] = '\a';
      }
    }
    write(sess->sd, full_str, f_len - 1); // -1 : do not include \0
    free(full_str);
    sess->fl_current = fl_current;
  } while ((fl_current = fl_current->next) != NULL);

  sess->fl_start = fl_start;
  write(sess->sd, page_info, strlen(page_info));
}

int directory_exists(const char *path) {
  struct stat info;

  if (stat(path, &info) != 0) {
    // stat() failed, probably doesn't exist
    return 0;
  }
  return S_ISDIR(info.st_mode); // true if it is a directory
}

void extract_names_from_hash(uint32_t file_hash, char *dirname, char *fname) {
  char hashed_full_name[9];
  sprintf(hashed_full_name, "%08x", file_hash);
  strncpy(dirname, hashed_full_name, 2);
  strncpy(fname, hashed_full_name + 2, 6);
  dirname[2] = 0;
  fname[6] = 0;
}

int32_t file_send_prepare(session *sess, char *line, server_data_t *s_d) {
  int32_t sd = sess->sd;
  i_get_file_db args = {.id = 0, .user_id = 0, .name = ""};
  int32_t file_d;
  size_t fsize;
  char hashed_dir_name[3];
  char hashed_name[7];
  char err_mes[256];
  uint32_t mlen;

  sscanf(line, "%*s %*s %s", args.name);
  sess->file = db_get_file(&args);

  if (sess->file == NULL)
    return -1;

  extract_names_from_hash(sess->file->hash, hashed_dir_name, hashed_name);
  sess->file->path = malloc(sizeof(STORAGE_FOLDER) + 2 + 9);
  sprintf(sess->file->path, "%s/%s/%s", STORAGE_FOLDER, hashed_dir_name,
          hashed_name);

  file_d = open(sess->file->path, O_RDONLY);

  if (file_d == -1) {
    fprintf(stdout, "ERROR: %d\n", errno);
    mlen = sprintf(err_mes, "Can't open file with id = %u\n", sess->file->id);
    write(sd, err_mes, mlen);
    return -2;
  }

  fsize = lseek(file_d, 0, SEEK_END);
  lseek(file_d, 0, SEEK_SET);

  if (sess->file->size != fsize) {
    mlen =
        sprintf(err_mes, "File with id = %u exists, but seems to be damaged\n",
                sess->file->id);
    write(sd, err_mes, mlen);
    return -3;
  }

  sess->file->rest = fsize;
  sess->fd = file_d;
  return 0;
}

int file_receive_prepare(session *sess, char *line, server_data_t *s_d) {
  int sd = sess->sd;
  char fname[64];
  size_t fsize;
  int perm;
  sscanf(line, "file upload \"%s %zd %d", fname, &fsize, &perm);
  sess->file = malloc(sizeof(s_file_t));
  sess->file->name = malloc(sizeof(char) * strlen(fname));
  sess->file->path = malloc(sizeof(STORAGE_FOLDER) + 2 + 9);
  sess->file->permissions = (char)perm;
  sess->file->description = NULL;
  strncpy(sess->file->name, fname, strlen(fname) - 1);
  sess->file->name[strlen(fname) - 1] = 0;

  char mes[256];
  char mes_len = 0;
  struct statvfs st_str;
  statvfs(".", &st_str);
  size_t available_space = st_str.f_bavail * st_str.f_bsize;
  if (strlen(sess->file->name) > st_str.f_namemax) {
    mes_len = sprintf(mes, "file name is tool long\n");
    write(sd, mes, mes_len);
    sess->state = OP_WAIT;
    clear_file_from_sess(sess);
    return -1;
  }
  if (fsize > available_space) {
    mes_len = sprintf(mes, "There is no space for such length! (%ld)\n", fsize);
    write(sd, mes, mes_len);
    clear_file_from_sess(sess);
    return -1;
  }

  char hashed_dir_name[3];
  char hashed_name[7];
  int seed = FILE_HASH_SEED;
  int file_d;

  for (;;) {
    MurmurHash3_x86_32(sess->file->name, strlen(sess->file->name), seed,
                       &sess->file->hash);

    extract_names_from_hash(sess->file->hash, hashed_dir_name, hashed_name);

    sprintf(sess->file->path, "%s/%s/%s", STORAGE_FOLDER, hashed_dir_name,
            hashed_name);

    chdir(STORAGE_FOLDER);

    if (!directory_exists(hashed_dir_name)) {
      mkdir(hashed_dir_name, 0700);
    }
    chdir("../");

    file_d = open(sess->file->path, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (file_d == -1) {
      if (errno == EEXIST) {
        seed++;
        continue;
      }
      mes_len = sprintf(mes, "Can't create file with such name: \"%s\"\n",
                        sess->file->name);
      write(sd, mes, mes_len);
      clear_file_from_sess(sess);
      sess->file = NULL;
      return -1;
    }
    break;
  }

  mes_len = sprintf(mes, "accept");
  sess->fd = file_d;
  sess->file->size = fsize;
  sess->file->rest = fsize;
  write(sd, mes, mes_len);
  return 0;
}

void clear_file_from_sess(session *s) {
  s_file_t *sf = s->file;
  if (sf->name != NULL) {
    free(sf->name);
    sf->name = NULL;
  }
  if (sf->path != NULL) {
    free(sf->path);
    sf->path = NULL;
  }
  if (sf->description != NULL) {
    free(sf->description);
    sf->description = NULL;
  }
  if (s->fd > -1) {
    close(s->fd);
    s->fd = -1;
  }
  free(sf);
  s->file = NULL;
}

void file_load(session *sess, enum f_actions f_action) {
  int source_d = f_action == F_DOWNLOAD ? sess->fd : sess->sd;
  int dest_d = f_action == F_DOWNLOAD ? sess->sd : sess->fd;
  char buf[INBUFSIZE];
  int rlen = read(source_d, buf, INBUFSIZE);
  if (rlen == 0) {
    if (sess->file->rest) {
      if (f_action == F_DOWNLOAD) {
        fprintf(stderr, "Error downloading file %s!\n", sess->file->name);
      } else {
        fprintf(stderr, "Error uploading file %s!\n", sess->file->name);
        unlink(sess->file->path); /* remove file */
      }
      clear_file_from_sess(sess);
    }
    sess->state = OP_WAIT;
    return;
  }
  write(dest_d, buf, rlen);
  sess->file->rest -= rlen;
  if (!sess->file->rest) {
    if (f_action == F_DOWNLOAD) {
      printf("File %s is downloaded from the server\n", sess->file->name);
      clear_file_from_sess(sess);
    } else {
      printf("File %s is uploaded to the server\n", sess->file->name);
    }
    if (f_action == F_UPLOAD) {
      sess->state = OP_UPLOAD_DESCRIPTION;
    } else {
      sess->state = OP_WAIT;
    }
  }
}

size_t get_file_size(char *dir_n, char *file_n) {
  char filepath[256];
  int dn_len;
  size_t fsize;
  strcpy(filepath, dir_n);
  dn_len = strlen(filepath);
  strcat(filepath + dn_len, file_n);

  int file_d = open(filepath, O_RDONLY);

  if (file_d == -1)
    return 0;

  fsize = lseek(file_d, 0, SEEK_END);
  lseek(file_d, 0, SEEK_SET);

  close(file_d);
  return fsize;
}

/* returns 1 if there is :END: ; 0 if opposite */
int file_upload_description(session *sess, char *line, server_data_t *s_d) {
  for (;;) {
    if (line != NULL && sess->file->description == NULL) { /* first query */
      sess->file->description = malloc(strlen(line) + 1);
      strcpy(sess->file->description, line);
    } else { /* subsequent queries */
      if (line == NULL) {
        if (sess->buf_used > 0) {
          query_extract_from_buf(sess, &line);
        } else {
          return 0;
        }
      }
      sess->file->description =
          realloc(sess->file->description,
                  strlen(sess->file->description) + strlen(line) + 1);
      strcat(sess->file->description, line);
    }
    char *desc_end = strstr(line, ":END:");
    if (desc_end != NULL) {
      sess->file->description[strlen(sess->file->description) - 6] =
          0; // cut the :END:
      return 1;
    }
    line = NULL;
  }
}