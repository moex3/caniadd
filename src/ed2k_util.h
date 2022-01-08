#ifndef _ED2K_UTIL_H
#define _ED2K_UTIL_H
#include <sys/stat.h>
#include <stdint.h>

#include "error.h"

typedef enum error (*ed2k_util_fn)(const char *path, const uint8_t *hash,
        const struct stat *st, void *data);
/*
 * If this returns ED2KUTIL_DONTHASH, then skip the hashing,
 * and the post_hash function
 */
typedef enum error (*ed2k_util_prehash_fn)(const char *path,
        const struct stat *st, void *data);

struct ed2k_util_opts {
    ed2k_util_fn post_hash_fn;
    ed2k_util_prehash_fn pre_hash_fn;
    void *data;
};

/*
 * Given a path (file or directory) calculate the ed2k
 * hash for the file(s), and call opts.post_hash_fn if not NULL
 * if opts.pre_hash_fn is not NULL, then also call that before the hashing
 *
 * If fn returns any error, the iteration will stop, and this
 * function will return with that error code.
 */
enum error ed2k_util_iterpath(const char *path, const struct ed2k_util_opts *opts);

#endif /* _ED2K_UTIL_H */
