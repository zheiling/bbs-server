#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
enum pl_type {
    pl_info,
    pl_success,
    pl_fail,
    pl_warning,
    pl_error,
    pl_fatal,
};

int print_log(FILE * output, enum pl_type type, const char *format_message, ...);
int init_log_file();
void close_log_file();

#endif