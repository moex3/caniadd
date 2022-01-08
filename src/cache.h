#ifndef _CACHE_H
#define _CACHE_H
#include <stdint.h>
#include <stdbool.h>

#include "error.h"
#include "ed2k.h"

struct cache_entry {
    uint64_t lid, fsize;
    char *fname;
    uint8_t ed2k[ED2K_HASH_SIZE];
};

/*
 * Init tha cache
 */
enum error cache_init();

/*
 * Free tha cache
 */
void cache_free();

/*
 * Add a new mylist entry to the cache
 */
enum error cache_add(uint64_t lid, const char *fname,
        uint64_t fsize, const uint8_t *ed2k);

/*
 * Get a cache entry
 *
 * out_ce can be NULL. Useful, if we only want
 * to check if the entry exists or not.
 */
enum error cache_get(const char *fname, uint64_t size,
        struct cache_entry *out_ce);

#endif /* _CACHE_H */
