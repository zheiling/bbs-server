#include "file_p.h"
#include "libs/murmur3/murmur3.h"
#include "main.h"
#include "session.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stddef.h>
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

void file_list(session *sess, server_data_t *s_d) {
  // TODO: change of format (rewrite in C++)
  char list_end[] = "list_end\n";
  fl_t *current;
  current = s_d->fl_start;
  char item_h[256];
  do {
    int n_len = strlen(sess->uname);
    if (strncmp(current->owner, sess->uname, n_len) && !current->permissions) {
      continue;
    }
    int h_len = sprintf(item_h, "%s %zu %s", current->name, current->size,
                        current->owner);
    int d_len = strlen(current->description);
    int f_len = h_len + d_len + 2;
    char *full_str = malloc(sizeof(char) * f_len);
    sprintf(full_str, "%s %s", item_h, current->description);
    int i;
    for (i = h_len; i < h_len + d_len; i++) {
      if (full_str[i] == '\n') {
        full_str[i] = '\a';
      }
    }
    write(sess->sd, full_str, f_len - 1); // -1 : do not include \0
    free(full_str);
  } while ((current = current->next) != NULL);
  write(sess->sd, list_end, sizeof(list_end));
}

int file_send_prepare(session *sess, char *line, server_data_t *s_d) {
  // TODO: рефактор в C++
  int sd = sess->sd;
  char fname[32];
  sscanf(line, "%*s %*s %s", fname);
  char filepath[256];
  sprintf(filepath, "%s/%s", STORAGE_FOLDER, fname);
  size_t fsize;
  char st_message[256];

  int file_d = open(filepath, O_RDONLY);

  if (file_d == -1) {
    fprintf(stdout, "ERROR: %d\n", errno);
    char err_mes[256];
    int mlen;
    mlen = sprintf(err_mes, "Can't open file named \"%s\"\n", fname);
    write(sd, err_mes, mlen);
    return -1;
  }

  fsize = lseek(file_d, 0, SEEK_END);
  lseek(file_d, 0, SEEK_SET);

  sess->fd = file_d;
  sess->fsize = fsize;
  sess->f_rest = fsize;
  sess->fname = malloc(strlen(fname));
  int fname_len = strlen(fname) - 1;
  strncpy(sess->fname, fname, fname_len);
  sess->fname[fname_len] = 0;
  return 0;
}

int directory_exists(const char *path) {
  struct stat info;

  if (stat(path, &info) != 0) {
    // stat() failed, probably doesn't exist
    return 0;
  }
  return S_ISDIR(info.st_mode); // true if it's a directory
}

void extract_names_from_hash(uint32_t file_hash, char *dirname, char *fname) {
  char hashed_full_name[9];
  sprintf(hashed_full_name, "%08x", file_hash);
  strncpy(dirname, hashed_full_name, 2);
  strncpy(fname, hashed_full_name + 2, 6);
  dirname[2] = 0;
  fname[6] = 0;
}

int file_receive_prepare(session *sess, char *line, server_data_t *s_d) {
  int sd = sess->sd;
  char fname[64];
  size_t fsize;
  int perm;
  sscanf(line, "file upload \"%s %zd %d", fname, &fsize, &perm);
  sess->fname = malloc(sizeof(char) * strlen(fname));
  sess->f_perm = (char)perm;
  strncpy(sess->fname, fname, strlen(fname) - 1); // remove the last \"

  char mes[256];
  char mes_len = 0;
  struct statvfs st_str;
  statvfs(".", &st_str);
  size_t available_space = st_str.f_bavail * st_str.f_bsize;
  if (strlen(sess->fname) > st_str.f_namemax) {
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
    MurmurHash3_x86_32(sess->fname, strlen(sess->fname), FILE_HASH_SEED,
                       &(sess->hash));

    extract_names_from_hash(sess->hash, hashed_dir_name, hashed_name);

    sess->fpath = malloc(sizeof(STORAGE_FOLDER) + 2 + 9);
    sprintf(sess->fpath, "%s/%s/%s", STORAGE_FOLDER, hashed_dir_name,
            hashed_name);

    if (!directory_exists(hashed_dir_name)) {
      mkdir(hashed_dir_name, 0700);
    }
    chdir(hashed_dir_name);

    file_d = open(sess->fpath, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (file_d == -1) {
      if (errno == EEXIST) {
        seed++;
        free(sess->fpath);
        sess->fpath = NULL;
        continue;
      }
      mes_len = sprintf(mes, "Can't create file with such name: \"%s\"\n",
                        sess->fname);
      write(sd, mes, mes_len);
      clear_file_from_sess(sess);
      return -1;
    }
  }

  mes_len = sprintf(mes, "accept");
  sess->fd = file_d;
  sess->fsize = fsize;
  sess->f_rest = fsize;
  write(sd, mes, mes_len);
  return 0;
}

void clear_file_from_sess(session *sess) {
  if (sess->fname != NULL) {
    free(sess->fname);
    sess->fname = NULL;
  }
  if (sess->fpath != NULL) {
    free(sess->fpath);
    sess->fpath = NULL;
  }
  if (sess->fdesc != NULL) {
    free(sess->fdesc);
    sess->fdesc = NULL;
  }
  if (sess->fd > -1) {
    close(sess->fd);
  }
  sess->fd = -1;
  sess->fsize = 0;
  sess->f_rest = 0;
}

void file_download_upload(session *sess, enum f_actions f_action) {
  int source_d = f_action == F_DOWNLOAD ? sess->fd : sess->sd;
  int dest_d = f_action == F_DOWNLOAD ? sess->sd : sess->fd;
  char buf[INBUFSIZE];
  int rlen = read(source_d, buf, INBUFSIZE);
  if (rlen == 0) {
    if (sess->f_rest) {
      if (f_action == F_DOWNLOAD) {
        fprintf(stderr, "Error downloading file %s!\n", sess->fname);
      } else {
        fprintf(stderr, "Error uploading file %s!\n", sess->fname);
        unlink(sess->fpath); /* remove file */
      }
      clear_file_from_sess(sess);
    }
    sess->state = OP_WAIT;
    return;
  }
  write(dest_d, buf, rlen);
  sess->f_rest -= rlen;
  if (!sess->f_rest) {
    if (f_action == F_DOWNLOAD) {
      printf("File %s is downloaded from the server\n", sess->fname);
      clear_file_from_sess(sess);
    } else {
      printf("File %s is uploaded to the server\n", sess->fname);
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
    if (line != NULL && sess->fdesc == NULL) { /* first query */
      sess->fdesc = malloc(strlen(line) + 1);
      strcpy(sess->fdesc, line);
    } else { /* subsequent queries */
      if (line == NULL) {
        if (sess->buf_used > 0) {
          query_extract_from_buf(sess, &line);
        } else {
          return 0;
        }
      }
      sess->fdesc =
          realloc(sess->fdesc, strlen(sess->fdesc) + strlen(line) + 1);
      strcat(sess->fdesc, line);
    }
    char *desc_end = strstr(line, ":END:");
    if (desc_end != NULL) {
      return 1;
    }
  }
}

// TODO: синхронизовать c C++
void get_files_descriptions(server_data_t *s_d) {
  int fd = open(FILE_DESCRIPTIONS_NAME, O_RDONLY);
  if (-1 == fd) {
    perror(FILE_DESCRIPTIONS_NAME);
    exit(8);
  }
  int rlen;

  char buf[INBUFSIZE] = "";
  chdir(STORAGE_FOLDER);

  while ((rlen = read(fd, buf, FILEBUFSIZE)) > 0) {
    char *buf_pos = buf;
    char tmp[256];
    while (buf_pos[0] != '\0') {
      fl_t *fl_item = malloc(sizeof(fl_t));
      fl_item->next = NULL;

      /* file name */
      sscanf(buf_pos, "%s\n", tmp);
      buf_pos += strlen(tmp) + 1;
      fl_item->name = malloc(sizeof(char) * (strlen(tmp) + 1));
      strcpy(fl_item->name, tmp);

      /* file owner */
      sscanf(buf_pos, "%s\n", tmp);
      buf_pos += strlen(tmp) + 1;
      fl_item->owner = malloc(sizeof(char) * (strlen(tmp) + 1));
      strcpy(fl_item->owner, tmp);

      /* file permissions */
      sscanf(buf_pos, "%s\n", tmp);
      buf_pos += strlen(tmp) + 1;
      fl_item->permissions = atoi(tmp);

      /* file size */
      int fd = open(fl_item->name, O_RDONLY);
      if (-1 == fd) {
        perror(fl_item->name);
        free(fl_item->name);
        free(fl_item->owner);
        free(fl_item);
        buf_pos += strstr(buf_pos, ":END:") + 6 - buf_pos;
        continue;
      }
      fl_item->size = lseek(fd, 0, SEEK_END);
      close(fd);

      /* file description */
      int d_len = strstr(buf_pos, ":END:\n") - buf_pos;
      fl_item->description = malloc(d_len + 1);
      strncpy(fl_item->description, buf_pos, d_len);
      fl_item->description[d_len] = 0;
      buf_pos += d_len + 6;
      if (s_d->fl_start == NULL) {
        s_d->fl_start = fl_item;
        s_d->fl_current = fl_item;
      } else {
        s_d->fl_current->next = fl_item;
        s_d->fl_current = fl_item;
      }
    }
  }
  close(fd);
  chdir("..");
}