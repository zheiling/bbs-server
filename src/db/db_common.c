#include <endian.h>
#include <openssl/sha.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void SHA256_raw_to_string(const unsigned char *passHashed, char *restrict out) {
  int i;
  for (i = 0; i < 4; i++) {
    uint64_t *num_pointer = (uint64_t *)(passHashed + i * 8);
    sprintf(out + 16 * i, "%016lx", htobe64(*num_pointer));
  }
}

void string_to_SHA256(const char *str, char *restrict out) {
  unsigned char md[SHA256_DIGEST_LENGTH];
  unsigned char *passHashed = SHA256((unsigned char *)str, strlen(str), md);
  SHA256_raw_to_string(passHashed, out);
}