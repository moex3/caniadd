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
        uint64_t lid;
        const char *arg = config_get_nonopt(i);

        if (sscanf(arg, "%lu", &lid) != 1) {
            uio_error("Argument '%s' is not an integer. Skipping", arg);
            continue;
        }

        err = api_cmd_mylistmod(lid, &mopt, &res);
        if (err != NOERR)
            break;

        if (res.code == APICODE_NO_SUCH_MYLIST_ENTRY) {
            uio_error("No mylist entry with id: '%lu'", lid);
        }
    }
    return err;
}

