#ifndef DB_COMMON_H
#define DB_COMMON_H
void SHA256_raw_to_string(const unsigned char *passHashed, char *restrict out);
void string_to_SHA256(const char *str, char *restrict out);
#endif