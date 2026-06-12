#include "utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
  switch (type) {
  case pl_info:
    uff_str = "%s %s";
    break;
  case pl_warning:
    uff_str = "%s Warning: %s";
    break;
  case pl_error:
    uff_str = "%s Error: %s";
    break;
  case pl_fatal:
    uff_str = "%s Fatal: %s";
    break;
  }
  char *uf_str = malloc(strlen(date_preffix) + 16 + strlen(format_message) + 4);
  sprintf(uf_str, uff_str, date_preffix, format_message);
  va_start(args, format_message);
  ret = vfprintf(stdout, uf_str, args);
  va_end(args);
  return ret;
}