#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include <ftw.h>

#include "util.h"
#include "error.h"
#include "uio.h"

void util_byte2hex(const uint8_t* bytes, size_t bytes_len,
        bool uppercase, char* out)
{
    const char* hex = (uppercase) ? "0123456789ABCDEF" : "0123456789abcdef";
    for (size_t i = 0; i < bytes_len; i++) {
        *out++ = hex[bytes[i] >> 4];
        *out++ = hex[bytes[i] & 0xF];
    }
    *out = '\0';
}

void util_hex2byte(const char *str, uint8_t* out_bytes)
{
    while (*str) {
        if (*str >= '0' && *str <= '9')
            *out_bytes = (*str - '0') << 4;
        if (*str >= 'A' && *str <= 'F')
            *out_bytes = (*str - ('A' - 10)) << 4;
        else
            *out_bytes = (*str - ('a' - 10)) << 4;

        str++;

        if (*str >= '0' && *str <= '9')
            *out_bytes |= (*str - '0');
        if (*str >= 'A' && *str <= 'F')
            *out_bytes |= (*str - ('A' - 10));
        else
            *out_bytes |= (*str - ('a' - 10));

        out_bytes++;
        str++;
    }
}

const char *util_get_home()
{
    const char *home_env = getenv("HOME");
    return home_env; /* TODO this can be null, use other methods as fallback */
}

char *util_basename(const char *fullpath)
{
    char *name_part = strrchr(fullpath, '/');
    if (name_part)
        name_part++;
    else
        name_part = (char*)fullpath;
    return name_part;
}

uint64_t util_timespec_diff(const struct timespec *past,
        const struct timespec *future)
{
    int64_t sdiff = future->tv_sec - past->tv_sec;
    int64_t nsdiff = future->tv_nsec - past->tv_nsec; 
    return sdiff * 1000 + (nsdiff / 1000000);
}

uint64_t util_iso2unix(const char *isotime)
{
    struct tm tm = {0};
    char *ms = strptime(isotime, "%Y-%m-%d", &tm);

    if (!ms || (*ms != '\0' && *ms != ' ' && *ms != 'T'))
        return 0;
    if (*ms == '\0')
        ms = "T00:00:00";

    ms = strptime(ms + 1, "%H:%M", &tm);
    if (!ms || (*ms != '\0' && *ms != ':'))
        return 0;
    if (*ms == '\0')
        ms = ":00";

    ms = strptime(ms + 1, "%S", &tm);
    if (!ms)
        return 0;

    return mktime(&tm);
}

/* nftw doesn't support passing in user data to the callback function :/ */
static util_itercb global_iterpath_cb = NULL;
static void *global_iterpath_data = NULL;

static int util_iterpath_walk(const char *fpath, const struct stat *sb,
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
        return ERR_ITERPATH;
    }

    return global_iterpath_cb(fpath, sb, global_iterpath_data);
}

enum error util_iterpath(const char *path, util_itercb cb, void *data)
{
    assert(global_iterpath_cb == NULL);
    enum error ret = ERR_ITERPATH;
    struct stat ts;

    if (stat(path, &ts) != 0) {
        uio_error("Stat failed for path: '%s' (%s)",
                path, strerror(errno));
        goto error;
    }

    if (S_ISREG(ts.st_mode)) {
        /* If the path is a regular file, call the cb once */
        ret = cb(path, &ts, data);
        goto end;
    } else if (S_ISDIR(ts.st_mode)) {
        global_iterpath_cb = cb;
        global_iterpath_data = data;

        /* If a directory, walk over it */
        ret = nftw(path, util_iterpath_walk, 20, 0);
        if (ret == -1) {
            uio_error("nftw failure");
            goto error;
        }
        goto end;
    }

    uio_error("Unsupported file type: %d", ts.st_mode & S_IFMT);
end:
    global_iterpath_cb = global_iterpath_data = NULL;
    return ret;
error:
    ret = ERR_ITERPATH;
    goto end;
}
