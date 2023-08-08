#include <sys/stat.h>
#include <stdio.h>
#include <stdbool.h>

#include "cmd.h"
#include "error.h"
#include "uio.h"
#include "config.h"
#include "ed2k_util.h"
#include "ed2k.h"
#include "util.h"

struct cmd_ed2k_opts {
    bool link;
};

static enum error cmd_ed2k_output(const char *path, const uint8_t *hash,
        const struct stat *st, void *data)
{
    struct cmd_ed2k_opts *eo = data;
    char buff[ED2K_HASH_SIZE * 2 + 1];
    bool upcase = eo->link;

    util_byte2hex(hash, ED2K_HASH_SIZE, upcase, buff);
    if (eo->link) {
        char *name_part = util_basename(path);

        printf("ed2k://|file|%s|%ld|%s|/\n", name_part, st->st_size, buff);
    } else {
        printf("%s\t%s\n", buff, path);
    }
    return NOERR;
}

enum error cmd_ed2k(void *data)
{
    struct cmd_ed2k_opts opts = {0};
    struct ed2k_util_opts ed2k_opts = {
        .post_hash_fn = cmd_ed2k_output,
        .data = &opts,
    };
    bool *link;
    enum error err = NOERR;
    int fcount;

    fcount = config_get_nonopt_count();
    if (fcount == 0) {
        uio_error("No files specified");
        return ERR_CMD_ARG;
    }

    if (config_get_subopt("ed2k", "link", (void**)&link) == NOERR)
        opts.link = *link;

    for (int i = 0; i < fcount; i++) {
        ed2k_util_iterpath(config_get_nonopt(i), &ed2k_opts);
        /* Above may fail if the path doesn't exists or smth, but still continue */
    }
    return err;
}

