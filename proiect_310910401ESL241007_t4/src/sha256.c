#include "sha256.h"
#include <openssl/sha.h>
#include <stdio.h>

int compute_sha256(const char *path, unsigned char hash[32])
{
    FILE *f = fopen(path, "rb");

    if (!f) {
        return -1;
    }

    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    unsigned char buffer[4096];
    size_t n;

    while ((n = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        SHA256_Update(&ctx, buffer, n);
    }

    SHA256_Final(hash, &ctx);

    fclose(f);

    return 0;
}
