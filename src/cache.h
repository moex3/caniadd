#ifndef _CACHE_H
#define _CACHE_H
#include <stdint.h>
#include <stdbool.h>

#include "error.h"
#include "ed2k.h"
#include "api.h"

enum cache_select {
    CACHE_S_ALL = 0,
    CACHE_S_LID = 1 << 0,
    CACHE_S_FNAME = 1 << 1,
    CACHE_S_FSIZE = 1 << 2,
    CACHE_S_ED2K = 1 << 3,
    CACHE_S_WATCHED = 1 << 4,
    CACHE_S_WATCHDATE = 1 << 5,
    CACHE_S_STATE = 1 << 6,
    CACHE_S_MODDATE = 1 << 7,
};

struct cache_entry {
    uint64_t lid, fsize, wdate, moddate;
    /* free() if requested */
    char *fname;
    uint8_t ed2k[ED2K_HASH_SIZE];
    bool watched;
    uint16_t state;
};

/*
 * Init tha cache
 */
enum error cache_init();

/*
 * Is the cache already setup or not?
 */
bool cache_is_init();

/*
 * Free tha cache
 */
void cache_free();

/*
 * Add a new mylist entry to the cache
 */
enum error cache_add(uint64_t lid, const char *fname,
        uint64_t fsize, const uint8_t *ed2k, uint64_t watchdate,
        enum mylist_state state);

/*
 * Update an already existing cache entry
 */
enum error cache_update();

/*
 * Get a cache entry
 *
 * sel is the columns to select
 * If 0, everything is selected and returned.
 * Can be ORed together
 *
 * out_ce can be NULL. Useful, if we only want
 * to check if the entry exists or not.
 */
enum error cache_get(const char *fname, uint64_t fsize, enum cache_select sel,
        struct cache_entry *out_ce);

/*
 * Does an entry exists?
 */
bool cache_exists(const char *fname, uint64_t size);

#endif /* _CACHE_H */
