#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "cmd.h"
#include "error.h"
#include "uio.h"
#include "api.h"
#include "config.h"
#include "cache.h"
#include "util.h"

bool did_cache_init = false;

uint64_t cmd_modify_arg_parse(const char *str, uint64_t *out_wdate)
{
    uint64_t val;
    struct cache_entry ce;
    enum error err;
    const char *fn_st = strrchr(str, '/');
    const char *size_st = strchr(str, '/');

    *out_wdate = 0;

    /* Skip the '/' */
    if (fn_st)
        fn_st++;
    if (size_st)
        size_st++;

    if (fn_st == size_st) {
        /* size/filename format */
        size_st = str;
    } else {
        size_t timelen = size_st - str - 1; /* -1 for the skipped '/' */
        char timstr[timelen + 1];

        memcpy(timstr, str, timelen);
        timstr[timelen] = '\0';

        *out_wdate = util_iso2unix(timstr);
    }

    if (sscanf(size_st, "%lu", &val) != 1)
        return 0;
    if (!fn_st && !size_st) /* Only lid format */
        return val;

    if (!cache_is_init()) {
        if (cache_init() != NOERR)
            return 0;
        did_cache_init = true;
    }

    err = cache_get(fn_st, val, CACHE_S_LID, &ce);
    if (err != NOERR)
        return 0;
    return ce.lid;
}

static enum error cmd_modify_api_send(uint64_t lid, struct api_mylistadd_opts *opts)
{
    struct api_result res;
    enum error err = api_cmd_mylistmod(lid, opts, &res);
    if (err != NOERR)
        return err;

    if (res.code == APICODE_NO_SUCH_MYLIST_ENTRY) {
        uio_error("No mylist entry with id: '%lu'", lid);
        return ERR_UNKNOWN;
    }

#if 0
    if (!cache_is_init()) {
        if ((err = cache_init()) != NOERR)
            return err;
        did_cache_init = true;
    }
#endif

    err = cache_update(lid, opts);
    if (err == NOERR) {
        uio_user("Successfully updated entry with lid:%lu", lid);
    }

    return err;
}

static enum error cmd_modify_file_cb(const char *path, const struct stat *fstat, void *data)
{
    struct cache_entry ce;
    enum error err;
    struct api_mylistadd_opts *opts = data;
    char *basename = util_basename(path);

    uio_debug("Modifying mylist by file: %s", basename);

    /* Get the mylist id from the cache */
    err = cache_get(basename, fstat->st_size, CACHE_S_LID, &ce);
    if (err != NOERR) {
        uio_error("Failed to get file from cache: %lu:%s. Maybe we should add it now?",
                fstat->st_size, basename);
        return err;
    }

    /* We have the mylistid, so actually send the change to the api */
    return cmd_modify_api_send(ce.lid, opts);
}

static enum error cmd_modify_by_file(const char *fpath, struct api_mylistadd_opts *opts)
{
    return util_iterpath(fpath, cmd_modify_file_cb, opts);
}

static enum error cmd_modify_by_mylistid(const char *arg, struct api_mylistadd_opts *opts)
{
    struct api_mylistadd_opts local_opts = *opts;
    uint64_t wdate, lid;

    lid = cmd_modify_arg_parse(arg, &wdate);

    if (lid == 0) {
        uio_error("Argument '%s' is not valid. Skipping", arg);
        return ERR_UNKNOWN;
    }

    /* If info is present in the argument line set it for the update */
    if (wdate != 0) {
        local_opts.watched = true;
        local_opts.watched_set = true;

        local_opts.wdate = wdate;
        local_opts.wdate_set = true;
    }

    return cmd_modify_api_send(lid, &local_opts);
}

static enum error cmd_modify_process_arg(const char *arg, struct api_mylistadd_opts *opts)
{
    struct stat fs;

    /* Check if the given path is a file/directory */
    if (stat(arg, &fs) == 0) {
        /* File exists handle with the file handler */
        return cmd_modify_by_file(arg, opts);
    }

    /* If stat failed */
    int eno = errno;
    if (eno != ENOENT) {
        /* Failed for some other reason apart from file not existing */
        uio_warning("Stat for file '%s' failed with: %s", arg, strerror(eno));
        return ERR_UNKNOWN;
    }
    /* File simply not found, try the with the mylistid syntax */

    return cmd_modify_by_mylistid(arg, opts);

}

enum error cmd_modify(void *data)
{
    struct api_mylistadd_opts mopt = {0};
    bool *watched;
    const char **wdate_str;
    enum error err = NOERR;
    int fcount;

    fcount = config_get_nonopt_count();

    if (config_get_subopt("modify", "watched", (void**)&watched) == NOERR && *watched) {
        mopt.watched = *watched;
        mopt.watched_set = true;

        if (config_get_subopt("modify", "wdate", (void**)&wdate_str) == NOERR) {
            uint64_t wdate = util_iso2unix(*wdate_str);

            if (wdate == 0) {
                uio_error("Invalid time value: '%s'", *wdate_str);
                return ERR_CMD_ARG;
            }
            mopt.wdate = wdate;
            mopt.wdate_set = true;
        }
    }

    for (int i = 0; i < fcount; i++) {
        const char *arg = config_get_nonopt(i);

        cmd_modify_process_arg(arg, &mopt);
    }

#if 0
    if (did_cache_init) {
        did_cache_init = false;
        cache_free();
    }
#endif
    return err;
}


enum error cmd_modify_argcheck()
{
    if (config_get_nonopt_count() == 0) {
        uio_error("No arguments given for modify");
        return ERR_CMD_ARG;
    }
    return NOERR;
}
