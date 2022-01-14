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

uint64_t cmd_modify_getlid(const char *str)
{
    uint64_t val;
    const char *sep = strchr(str, '|');
    struct cache_entry ce;
    enum error err;

    if (sscanf(str, "%lu", &val) != 1)
        return 0;
    if (!sep)
        return val;

    if (!cache_is_init()) {
        if (cache_init() != NOERR)
            return 0;
        did_cache_init = true;
    }

    err = cache_get(sep + 1, val, CACHE_S_LID, &ce);
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

    for (int i = 0; i < fcount; i++) {
        struct api_result res;
        const char *arg = config_get_nonopt(i);
        uint64_t lid = cmd_modify_getlid(arg);

        if (lid == 0) {
            uio_error("Argument '%s' is not valid. Skipping", arg);
            continue;
        }

        err = api_cmd_mylistmod(lid, &mopt, &res);
        if (err != NOERR)
            break;

        if (res.code == APICODE_NO_SUCH_MYLIST_ENTRY) {
            uio_error("No mylist entry with id: '%lu'", lid);
        }
    }

    if (did_cache_init) {
        did_cache_init = false;
        cache_free();
    }
    return err;
}

