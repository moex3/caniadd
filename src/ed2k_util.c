#define _XOPEN_SOURCE 500
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <ftw.h>

#include "ed2k.h"
#include "ed2k_util.h"
#include "uio.h"

static struct ed2k_util_opts l_opts;

static enum error ed2k_util_hash(const char *file_path, blksize_t blksize,
        const struct stat *st)
{
    unsigned char buf[blksize], hash[ED2K_HASH_SIZE];
    struct ed2k_ctx ed2k;
    FILE *f;
    size_t read_len;

    if (l_opts.pre_hash_fn) {
        enum error err = l_opts.pre_hash_fn(file_path, st, l_opts.data);
        if (err == ED2KUTIL_DONTHASH)
            return NOERR;
        else if (err != NOERR)
            return err;
    }

    f = fopen(file_path, "rb");
    if (!f) {
        uio_error("Failed to open file: %s (%s)", file_path, strerror(errno));
        return ERR_ED2KUTIL_FS;
    }

    ed2k_init(&ed2k);
    read_len = fread(buf, 1, sizeof(buf), f);
    while (read_len > 0) {
        ed2k_update(&ed2k, buf, read_len);
        read_len = fread(buf, 1, sizeof(buf), f);
    }
    // TODO check if eof or error

    ed2k_final(&ed2k, hash);
    fclose(f);

    if (l_opts.post_hash_fn)
        return l_opts.post_hash_fn(file_path, hash, st, l_opts.data);
    return NOERR;
}

static int ed2k_util_walk(const char *fpath, const struct stat *sb,
        int typeflag, struct FTW *ftwbuf)
{
    if (typeflag == FTW_DNR) {
        uio_error("Cannot read directory '%s'. Skipping", fpath);
        return NOERR;
    }
    if (typeflag == FTW_D)
        return NOERR;
    if (typeflag != FTW_F) {
        uio_error("Unhandled error '%d'", typeflag);
        return ERR_ED2KUTIL_UNSUP;
    }

    return ed2k_util_hash(fpath, sb->st_blksize, sb);
}

enum error ed2k_util_iterpath(const char *path, const struct ed2k_util_opts *opts)
{
    struct stat ts;

    if (stat(path, &ts) != 0) {
        uio_error("Stat failed for path: '%s' (%s)",
                path, strerror(errno));
        return ERR_ED2KUTIL_FS;
    }

    l_opts = *opts;

    if (S_ISREG(ts.st_mode)) {
        return ed2k_util_hash(path, ts.st_blksize, &ts);
    } else if (S_ISDIR(ts.st_mode)) {
        int ftwret = nftw(path, ed2k_util_walk, 20, 0);
        if (ftwret == -1) {
            uio_error("nftw failure");
            return ERR_ED2KUTIL_FS;
        }
        return ftwret;
    }

    uio_error("Unsupported file type: %d", ts.st_mode & S_IFMT);
    return ERR_ED2KUTIL_UNSUP;
}
