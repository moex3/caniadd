#include <assert.h>

#include "ed2k.h"

/* https://wiki.anidb.net/Ed2k-hash */
/*   This is using the red method   */

#define ED2K_CHUNK_SIZE 9728000

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

void ed2k_init(struct ed2k_ctx *ctx)
{
    md4_init(&ctx->chunk_md4_ctx);
    md4_init(&ctx->hash_md4_ctx);
    ctx->byte_count = 0;
}

static void ed2k_hash_chunk(struct ed2k_ctx *ctx)
{
    unsigned char chunk_hash[MD4_DIGEST_SIZE];

    md4_final(&ctx->chunk_md4_ctx, chunk_hash);
    md4_update(&ctx->hash_md4_ctx, chunk_hash, sizeof(chunk_hash));
    md4_init(&ctx->chunk_md4_ctx);
}

void ed2k_update(struct ed2k_ctx *ctx, const void *data, size_t data_len)
{
    const char *bytes = (const char*)data;

    while (data_len) {
        size_t hdata_size = MIN(ED2K_CHUNK_SIZE -
                (ctx->byte_count % ED2K_CHUNK_SIZE), data_len);

        md4_update(&ctx->chunk_md4_ctx, bytes, hdata_size);
        ctx->byte_count += hdata_size;

        if (ctx->byte_count % ED2K_CHUNK_SIZE == 0)
            ed2k_hash_chunk(ctx);
        
        data_len -= hdata_size;
        bytes += hdata_size;
    }
}

void ed2k_final(struct ed2k_ctx *ctx, unsigned char *out_hash)
{
    struct md4_ctx *md_ctx;
    if (ctx->byte_count < ED2K_CHUNK_SIZE) {
        /* File has only 1 chunk, so return the md4 hash of that chunk */
        md_ctx = &ctx->chunk_md4_ctx;
    } else {
        /* Else hash the md4 hashes, and return that hash */
        ed2k_hash_chunk(ctx); /* Hash the last partial chunk here */
        md_ctx = &ctx->hash_md4_ctx;
    }

    md4_final(md_ctx, out_hash);
}

