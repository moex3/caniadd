#include <stdbool.h>

#include "cmd.h"
#include "error.h"
#include "config.h"
#include "api.h"
#include "uio.h"
#include "net.h"
#include "cache.h"

struct cmd_entry {
    bool need_api : 1; /* Does this command needs to connect to the api? */
    bool need_auth : 1; /* Does this command needs auth to the api? sets need_api */
    bool need_cache : 1; /* Does this cmd needs the file cache? */
    const char *arg_name; /* If this argument is present, execute this cmd */
    
    enum error (*argcheck)(void); /* Function to check argument correctness before calling fn */
    enum error (*fn)(void *data); /* The function for the command */
};

static const struct cmd_entry ents[] = {
    { .arg_name = "version", .fn = cmd_prog_version, },
    { .arg_name = "server-version", .fn = cmd_server_version, .need_api = true },
    { .arg_name = "uptime", .fn = cmd_server_uptime, .need_auth = true },
    { .arg_name = "ed2k", .fn = cmd_ed2k, },
    { .arg_name = "add", .fn = cmd_add, .argcheck = cmd_add_argcheck, .need_auth = true, .need_cache = true, },
    { .arg_name = "modify", .fn = cmd_modify, .argcheck = cmd_modify_argcheck, .need_auth = true, .need_cache = true, },
};
static const int32_t ents_len = sizeof(ents)/sizeof(*ents);

static enum error cmd_run_one(const struct cmd_entry *ent)
{
    enum error err = NOERR;

    if (ent->argcheck) {
        err = ent->argcheck();
        if (err != NOERR)
            goto end;
    }
    if (ent->need_cache) {
        err = cache_init();
        if (err != NOERR)
            goto end;
    }
    if (ent->need_api || ent->need_auth) {
        err = api_init(ent->need_auth);
        if (err != NOERR)
            return err;
    }

    void *data = NULL;
    err = ent->fn(data);

end:
    if (ent->need_api || ent->need_auth)
        api_free();
    if (ent->need_cache)
        cache_free();
    return err;
}

enum error cmd_main()
{
    for (int i = 0; i < ents_len; i++) {
        enum error err;
        bool *is_set;

        err = config_get(ents[i].arg_name, (void**)&is_set);
        if (err != NOERR && err != ERR_OPT_UNSET) {
            uio_error("Cannot get arg '%s' (%s)", ents[i].arg_name,
                    error_to_string(err));
            continue;
        }

        if (*is_set) {
            err = cmd_run_one(&ents[i]);
            return err;
        }
    }

    return ERR_CMD_NONE;
}
