#ifndef _ED2K_H
#define _ED2K_H
#include <stdint.h>

#include "md4.h"

#define ED2K_HASH_SIZE MD4_DIGEST_SIZE

struct ed2k_ctx {
    struct md4_ctx hash_md4_ctx, chunk_md4_ctx;
    uint64_t byte_count;
};

void ed2k_init(struct ed2k_ctx *ctx);
void ed2k_update(struct ed2k_ctx *ctx, const void *data, size_t data_len);
void ed2k_final(struct ed2k_ctx *ctx, unsigned char *out_hash);

#endif /* _ED2K_H */
