#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

char* base64_encode(uint8_t *data, size_t len, bool padding);
char* base64_url_encode(uint8_t *data, size_t len, bool padding);
void hmac_sha256_sign(uint8_t *data, size_t data_len, uint8_t *secret, size_t secret_len, uint8_t *res);
int uuid_from_string(char *str, uint8_t *res);
void uuid_to_string(uint8_t *uuid, char *res);

#endif /* UTIL_H */
