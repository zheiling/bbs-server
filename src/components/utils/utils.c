#include "utils.h"
#include <asm-generic/errno-base.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define LOGFILE_NAME "bbs-server.log"
/* COLORS */
#define INFO_COLOR "\033[0m"
#define WARN_COLOR "\033[0m\033[33m"
#define ERR_COLOR "\033[0m\033[31m"
#define FATAL_COLOR "\033[0m\033[31m\033[1m"
#define FATAL_COLOR "\033[0m\033[31m\033[1m"
#define SUCCESS_COLOR "\033[0m\033[32m"
#define FAIL_COLOR "\033[0m\033[35m"

static struct {
  int fd;
} file;

int open_log_file(char *path, int *_fd) {
  int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd == -1) {
    return errno;
  }
  *_fd = fd;
  return 0;
}

void close_log_file() { close(file.fd); }

/* TODO: create recursive backups 5 times */
int init_log_file() {
  int fd;
  int res = open_log_file(LOGFILE_NAME, &fd);
  if (res == EEXIST) {
    int res = open_log_file(LOGFILE_NAME ".old", &fd);
    if (res == EEXIST) {
      unlink(LOGFILE_NAME ".old");
    } else {
      close(fd);
    }
    rename(LOGFILE_NAME, LOGFILE_NAME ".old");
    res = open_log_file(LOGFILE_NAME, &fd);
    if (res != 0) {
      return -1;
    }
  }
  file.fd = fd;
  return fd;
}

int write_into_log_file(char *fmt, va_list args) {
  return vdprintf(file.fd, fmt, args);
}

int print_log(FILE *output, enum pl_type type, const char *format_message,
              ...) {
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  char date_preffix[128];
  const char *uff_str;
  int ret;
  va_list args;
  sprintf(date_preffix, "[%s] ", ctime(&t));
  strftime(date_preffix, 128, "[%d.%m.%y %T]", tm);
  const char *tcolor;
  switch (type) {
  case pl_info:
    uff_str = "%s%s %s";
    tcolor = INFO_COLOR;
    break;
  case pl_warning:
    uff_str = "%s%s Warning: %s";
    tcolor = WARN_COLOR;
    break;
  case pl_error:
    uff_str = "%s%s Error: %s";
    tcolor = ERR_COLOR;
    break;
  case pl_fatal:
    uff_str = "%s%s Fatal: %s";
    tcolor = FATAL_COLOR;
    break;
  case pl_success:
    uff_str = "%s%s %s";
    tcolor = SUCCESS_COLOR;
    break;
  case pl_fail:
    uff_str = "%s%s Fail: %s";
    tcolor = FAIL_COLOR;
    break;
  }
  char *uf_str = malloc(strlen(date_preffix) + 32 + strlen(format_message) + 4);
  sprintf(uf_str, uff_str, tcolor, date_preffix, format_message);
  va_start(args, format_message);
  ret = vfprintf(output, uf_str, args);
  write_into_log_file(uf_str + strlen(tcolor), args);
  free(uf_str);
  va_end(args);
  return ret;
}