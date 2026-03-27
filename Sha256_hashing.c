#include "sha256.h"
#include <stdio.h>
#include <string.h>

int main() {
    char text[256];
    printf("введите текст для хэширования: ");
    scanf("%255s", text);
    BYTE hash[SHA256_BLOCK_SIZE];
    SHA256_CTX ctx;

    sha256_init(&ctx);
    sha256_update(&ctx, (const BYTE *)text, strlen(text));
    sha256_final(&ctx, hash);

    printf("SHA-256 hash: ");
    for (int i = 0; i < SHA256_BLOCK_SIZE; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");

    return 0;
}
