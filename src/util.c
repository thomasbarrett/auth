#include <util.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <sha256.h>

void hmac_sha256_sign(uint8_t *data, size_t data_len, uint8_t *secret, size_t secret_len, uint8_t *res) {
    uint8_t key[64] = {0};
    if (secret_len > 64) {
        sha256(secret, secret_len, key);
    } else {
        memcpy(key, secret, secret_len);
    }

    uint8_t o_key_pad[96];
    uint8_t i_key_pad[64 + data_len];
    for (size_t i = 0; i < 64; i++) {
        o_key_pad[i] = key[i] ^ 0x5c;
        i_key_pad[i] = key[i] ^ 0x36;
    }
    memcpy(i_key_pad + 64, data, data_len);
    sha256(i_key_pad, 64 + data_len, o_key_pad + 64);
    sha256(o_key_pad, 96, res);
}

char* base64_encode(uint8_t *data, size_t len, bool padding) {
    size_t tmp_len = 3 * ((len + 2) / 3);
    size_t pad_len = tmp_len - len;
    size_t out_len = 4 * tmp_len / 3;
    char *output = calloc(out_len + 1, 1);
    if (output == NULL) return NULL;
    uint8_t tmp[tmp_len];
    memset(tmp, 0, tmp_len);
    memcpy(tmp, data, len);
    for (size_t i = 0; i < tmp_len; i += 3) {
        uint32_t tmp2 = (tmp[i] << 24) | (tmp[i + 1] << 16) | (tmp[i + 2] << 8);
        for (size_t j = 0; j < 4; j++) {
            uint8_t byte = (uint8_t) ((tmp2 & 0xFC000000U) >> 26);
            size_t idx = 4 * (i / 3) + j;
            if (byte < 26) {
                output[idx] = 'A' + byte;
            } else if (byte < 52) {
                output[idx] = 'a' + (byte - 26);
            } else if (byte < 62) {
                output[idx] = '0' + (byte - 52);
            } else if (byte == 62) {
                output[idx] = '+';
            } else if (byte == 63) {
                output[idx] = '/';
            }
            tmp2 <<= 6;
        }
    }
    if (padding) {
        if (pad_len > 0) output[out_len - 1] = '=';
        if (pad_len > 1) output[out_len - 2] = '=';
    } else {
        if (pad_len > 0) output[out_len - 1] = '\0';
        if (pad_len > 1) output[out_len - 2] = '\0';
    }
    return output;
}

char* base64_url_encode(uint8_t *data, size_t len, bool padding) {
    char *res = base64_encode(data, len, padding);
    for (char *i = res; *i != '\0'; i++) {
        if (*i == '+') *i = '-';
        if (*i == '/') *i = '_';
    }
    return res;
}

void uuid_to_string(uint8_t *uuid, char *res) {
    sprintf(res, 
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        uuid[0x0], uuid[0x1], uuid[0x2], uuid[0x3],
        uuid[0x4], uuid[0x5], uuid[0x6], uuid[0x7],
        uuid[0x8], uuid[0x9], uuid[0xa], uuid[0xb],
        uuid[0xc], uuid[0xd], uuid[0xe], uuid[0xf]
    );
}

int uuid_from_string(char *str, uint8_t *res) {
    int tmp[16] = {0};
    int err = sscanf(str, 
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        &tmp[0x0], &tmp[0x1], &tmp[0x2], &tmp[0x3],
        &tmp[0x4], &tmp[0x5], &tmp[0x6], &tmp[0x7],
        &tmp[0x8], &tmp[0x9], &tmp[0xa], &tmp[0xb],
        &tmp[0xc], &tmp[0xd], &tmp[0xe], &tmp[0xf]
    );
    if (err != 16) return -1;
    for (size_t i = 0; i < 16; i++) {
        res[i] = (uint8_t)(tmp[i]);
    }
    return 0;
}
