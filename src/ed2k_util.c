#define _XOPEN_SOURCE 500
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "util.h"
#include "ed2k.h"
#include "ed2k_util.h"
#include "uio.h"
#include "globals.h"

static enum error ed2k_util_hash(const char *file_path, const struct stat *st, void *data)
{
    off_t blksize = st->st_blksize;
    unsigned char buf[blksize], hash[ED2K_HASH_SIZE];
    struct ed2k_util_opts *opts = data;
    struct ed2k_ctx ed2k;
    enum error err;
    FILE *f;
    size_t read_len;
    int en;

    if (opts->pre_hash_fn) {
        err = opts->pre_hash_fn(file_path, st, opts->data);
        if (err == ED2KUTIL_DONTHASH)
            return NOERR;
        else if (err != NOERR)
            return err;
    }

    f = fopen(file_path, "rb");
    if (!f) {
        en = errno;

        uio_error("Failed to open file: %s (%s)", file_path, strerror(en));
        if (en == EINTR && should_exit)
            return ERR_SHOULD_EXIT;
        else
            return ERR_ITERPATH;
    }

    ed2k_init(&ed2k);
    read_len = fread(buf, 1, sizeof(buf), f);
    /* From my test, fread wont return anything special on signal interrupt */
    while (read_len > 0 && !should_exit) {
        ed2k_update(&ed2k, buf, read_len);
        read_len = fread(buf, 1, sizeof(buf), f);
    }
    if (should_exit) {
        err = ERR_SHOULD_EXIT;
        goto fail;
    }
    if (ferror(f)) { /* Loop stopped bcuz of error, not EOF */
        uio_error("Failure while reading file");
        err = ERR_ITERPATH;
        goto fail;
    }
    assert(feof(f));

    ed2k_final(&ed2k, hash);
    if (fclose(f) != 0) {
        en = errno;

        uio_debug("Fclose failed: %s", strerror(en));
        if (en == EINTR && should_exit)
            return ERR_SHOULD_EXIT;
        else
            return ERR_ITERPATH;
    }

    if (opts->post_hash_fn)
        return opts->post_hash_fn(file_path, hash, st, opts->data);
    return NOERR;

fail:
    if (f) /* We can't get a 2nd interrupt now */
        fclose(f);
    return err;
}

enum error ed2k_util_iterpath(const char *path, const struct ed2k_util_opts *opts)
{
    return util_iterpath(path, ed2k_util_hash, (void*)opts);
}
