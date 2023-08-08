#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

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

enum error cmd_modify(void *data)
{
    struct api_mylistadd_opts mopt = {0};
    bool *watched;
    const char **wdate_str;
    enum error err = NOERR;
    int fcount;

    fcount = config_get_nonopt_count();
    if (fcount == 0) {
        uio_error("No mylist ids specified");
        return ERR_CMD_ARG;
    }

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
        struct api_result res;
        struct api_mylistadd_opts l_opts = mopt;
        const char *arg = config_get_nonopt(i);
        uint64_t wdate;
        uint64_t lid = cmd_modify_arg_parse(arg, &wdate);

        if (lid == 0) {
            uio_error("Argument '%s' is not valid. Skipping", arg);
            continue;
        }

        if (wdate != 0) {
            l_opts.watched = true;
            l_opts.watched_set = true;

            l_opts.wdate = wdate;
            l_opts.wdate_set = true;
        }

        err = api_cmd_mylistmod(lid, &l_opts, &res);
        if (err != NOERR)
            break;

        if (res.code == APICODE_NO_SUCH_MYLIST_ENTRY) {
            uio_error("No mylist entry with id: '%lu'", lid);
            continue;
        }
        
        if (!cache_is_init()) {
            if ((err = cache_init()) != NOERR)
                return err;
            did_cache_init = true;
        }

        err = cache_update(lid, &l_opts);
        if (err != NOERR)
            break;
    }

    if (did_cache_init) {
        did_cache_init = false;
        cache_free();
    }
    return err;
}

