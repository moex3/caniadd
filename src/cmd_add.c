#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "cmd.h"
#include "error.h"
#include "uio.h"
#include "api.h"
#include "config.h"
#include "ed2k_util.h"
#include "cache.h"
#include "util.h"

enum error cmd_add_cachecheck(const char *path, const struct stat *st,
        void *data)
{
    const char *bname = util_basename(path);
    enum error err;

    err = cache_get(bname, st->st_size, NULL);
    if (err == NOERR) {
        /* We could get the entry, so it exists already */
        uio_user("This file (%s) with size (%lu) already exists in cache."
                " Skipping", bname, st->st_size);
        return ED2KUTIL_DONTHASH;
    } else if (err != ERR_CACHE_NO_EXISTS) {
        uio_error("Some error when trying to get from cache: %s",
                error_to_string(err));
        return ED2KUTIL_DONTHASH;
    }

    uio_user("Hashing %s", path);
    return NOERR;
}

enum error cmd_add_apisend(const char *path, const uint8_t *hash,
        const struct stat *st, void *data)
{
    struct api_result r;
    struct api_mylistadd_opts *mopt = (struct api_mylistadd_opts *)data;

    if (api_cmd_mylistadd(st->st_size, hash, mopt, &r)
            != NOERR)
        return ERR_CMD_FAILED;

    if (r.code == 310) {
        struct api_mylistadd_result *x = &r.mylistadd;

        uio_warning("File already added! Adding it to cache");
        uio_debug("File info: lid: %ld, fid: %ld, eid: %ld, aid: %ld,"
                " gid: %ld, date: %ld, viewdate: %ld, state: %d,"
                " filestate: %d\nstorage: %s\nsource: %s\nother: %s",
                x->lid, x->fid, x->eid, x->aid, x->gid, x->date, x->viewdate,
                x->state, x->filestate, x->storage, x->source, x->other);

        cache_add(x->lid, util_basename(path), st->st_size, hash);

        if (x->storage)
            free(x->storage);
        if (x->source)
            free(x->source);
        if (x->other)
            free(x->other);
        return NOERR;
    }
    if (r.code != 210) {
        uio_error("Mylistadd failure: %hu", r.code);
        return ERR_CMD_FAILED;
    }

    uio_user("Succesfully added!");
    uio_debug("New mylist id is: %ld", r.mylistadd.new_id);
    cache_add(r.mylistadd.new_id, util_basename(path), st->st_size, hash);

    return NOERR;
}

enum error cmd_add(void *data)
{
    struct api_mylistadd_opts mopt = {0};
    struct ed2k_util_opts ed2k_opts = {
        .pre_hash_fn = cmd_add_cachecheck,
        .post_hash_fn = cmd_add_apisend,
        .data = &mopt,
    };
    bool *watched;
    const char **wdate_str;
    enum error err = NOERR;
    int fcount;

    fcount = config_get_nonopt_count();
    if (fcount == 0) {
        uio_error("No files specified");
        return ERR_CMD_ARG;
    }

    if (config_get("watched", (void**)&watched) == NOERR && *watched) {
        mopt.watched = *watched;
        mopt.watched_set = true;

        if (config_get("wdate", (void**)&wdate_str) == NOERR) {
            uint64_t wdate = util_iso2unix(*wdate_str);

            if (wdate == 0) {
                uio_error("Invalid time value: '%s'", *wdate_str);
                return ERR_CMD_ARG;
            }
            mopt.wdate = wdate;
            mopt.wdate_set = true;
        }
    }
    mopt.state = MYLIST_STATE_INTERNAL;
    mopt.state_set = true;

    for (int i = 0; i < fcount; i++) {
        err = ed2k_util_iterpath(config_get_nonopt(i), &ed2k_opts);
        if (err != NOERR)
            break;
    }
    return err;
}

