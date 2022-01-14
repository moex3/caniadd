#include <stddef.h>
#include <inttypes.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
//#include <toml.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>

#include "config.h"
#include "error.h"
#include "util.h"

static int show_help(struct conf_entry *ce);
static int config_parse_file();
static enum error config_required_check();

static int config_set_str(struct conf_entry *ce, char *arg);
static int config_set_port(struct conf_entry *ce, char *arg);
static int config_set_bool(struct conf_entry *ce, char *arg);

//static int config_def_cachedb(struct conf_entry *ce);

//static int config_action_write_config(struct conf_entry *ce);

/* Everything not explicitly defined, is 0 */
/* If an option only has a long name, the short name also has to be
 * defined. For example, a number larger than UCHAR_MAX */
static struct conf_entry options[] = {
    { .l_name = "help", .s_name = 'h', .has_arg = no_argument,
        .action_func = show_help, .in_args = true,
        .type = OTYPE_ACTION, .handle_order = 0 },
/*
    { .l_name = "config-dir", .s_name = 'b', .has_arg = required_argument,
        .default_func = config_def_config_dir, .set_func = config_set_str,
        .in_args = true, .type = OTYPE_S, .handle_order = 0 },

    { .l_name = "default-download-dir", .s_name = 'd', .has_arg = required_argument,
        .default_func = config_def_default_download_dir, .set_func = config_set_str,
        .in_file = true, .in_args = true, .type = OTYPE_S, .handle_order = 1 },

    { .l_name = "port", .s_name = 'p', .has_arg = required_argument,
        .set_func = config_set_port, .value.hu = 21729, .value_is_set = true,
        .in_file = true, .in_args = true, .type = OTYPE_HU, .handle_order = 1 },

    { .l_name = "foreground", .s_name = 'f', .has_arg = no_argument,
        .set_func = config_set_bool, .value.b = false, .value_is_set = true,
        .in_args = true, .type = OTYPE_B, .handle_order = 1 },

    { .l_name = "write-config", .s_name = UCHAR_MAX + 1, .has_arg = no_argument,
        .action_func = config_action_write_config, .value_is_set = true,
        .in_args = true, .type = OTYPE_ACTION, .handle_order = 2 },

    { .l_name = "peer-id", .s_name = UCHAR_MAX + 2, .has_arg = required_argument,
        .default_func = config_def_peer_id, .type = OTYPE_S, .handle_order = 1 },
        */

    { .l_name = "username", .s_name = 'u', .has_arg = required_argument,
      .set_func = config_set_str, .in_args = true, .in_file = true,
      .type = OTYPE_S, .handle_order = 1 },

    { .l_name = "password", .s_name = 'p', .has_arg = required_argument,
      .set_func = config_set_str, .in_args = true, .in_file = true,
      .type = OTYPE_S, .handle_order = 1 },

    { .l_name = "port", .s_name = 'P', .has_arg = required_argument,
      .set_func = config_set_port, .in_args = true, .in_file = true,
      .type = OTYPE_HU, .handle_order = 1, .value.hu = 29937,
      .value_is_set = true },

    { .l_name = "api-server", .s_name = UCHAR_MAX + 1, .has_arg = required_argument,
      .set_func = config_set_str, .in_args = true, .in_file = true,
      .type = OTYPE_S, .handle_order = 1, .value.s = "api.anidb.net:9000",
      .value_is_set = true },

    { .l_name = "api-key", .s_name = 'k', .has_arg = required_argument,
      .set_func = config_set_str, .in_args = true, .in_file = true,
      .type = OTYPE_S, .handle_order = 1, },

    { .l_name = "save-session", .s_name = 's', .has_arg = no_argument,
      .set_func = config_set_bool, .in_args = true, .in_file = true,
      .type = OTYPE_B, .handle_order = 1, .value_is_set = true },

    { .l_name = "destroy-session", .s_name = 'S', .has_arg = no_argument,
      .set_func = config_set_bool, .in_args = true, .in_file = false,
      .type = OTYPE_B, .handle_order = 1, .value_is_set = true },

    { .l_name = "watched", .s_name = 'w', .has_arg = no_argument,
      .set_func = config_set_bool, .in_args = true,
      .type = OTYPE_B, .handle_order = 1, .value_is_set = true },

    { .l_name = "link", .s_name = 'l', .has_arg = no_argument,
      .set_func = config_set_bool, .in_args = true,
      .type = OTYPE_B, .handle_order = 1, .value_is_set = true },

    { .l_name = "cachedb", .s_name = 'd', .has_arg = required_argument,
      .set_func = config_set_str, .in_args = true, .in_file = true,
      .type = OTYPE_S, .handle_order = 1, /*.default_func = config_def_cachedb*/ },

    { .l_name = "debug", .s_name = 'D', .has_arg = no_argument,
      .set_func = config_set_bool, .in_args = true, .in_file = true,
      .type = OTYPE_B, .handle_order = 1, .value_is_set = true, },

    { .l_name = "wdate", .s_name = UCHAR_MAX + 4, .has_arg = required_argument,
      .set_func = config_set_str, .in_args = true, 
      .type = OTYPE_S, .handle_order = 1, },

    /*### cmd ###*/

    { .l_name = "server-version", .s_name = UCHAR_MAX + 2,
      .has_arg = no_argument, .set_func = config_set_bool, .in_args = true,
      .type = OTYPE_B, .handle_order = 1, .value_is_set = true },

    { .l_name = "version", .s_name = 'v',
      .has_arg = no_argument, .set_func = config_set_bool, .in_args = true,
      .type = OTYPE_B, .handle_order = 1, .value_is_set = true },

    { .l_name = "uptime", .s_name = UCHAR_MAX + 3,
      .has_arg = no_argument, .set_func = config_set_bool, .in_args = true,
      .type = OTYPE_B, .handle_order = 1, .value_is_set = true },

    { .l_name = "ed2k", .s_name = 'e',
      .has_arg = no_argument, .set_func = config_set_bool, .in_args = true,
      .type = OTYPE_B, .handle_order = 1 },

    { .l_name = "add", .s_name = 'a',
      .has_arg = no_argument, .set_func = config_set_bool, .in_args = true,
      .type = OTYPE_B, .handle_order = 1 },

    /* Arguments are either mylist id's, or file sizes and names
     *  in the format '[watch_date/]<size>/<filename>'. The filename can't contain
     *  '/' characters.  */
    { .l_name = "modify", .s_name = 'W',
      .has_arg = no_argument, .set_func = config_set_bool, .in_args = true,
      .type = OTYPE_B, .handle_order = 1 },

    /*{ .l_name = "stats", .s_name = UCHAR_MAX + 5,
      .has_arg = no_argument, .set_func = config_set_bool, .in_args = true,
      .type = OTYPE_B, .handle_order = 1, .value_is_set = true },*/

};

static const size_t options_count = sizeof(options) / sizeof(options[0]);
static const char **opt_argv = NULL;
static int opt_argc = 0;

static void config_build_getopt_args(char out_sopt[options_count * 2 + 1],
        struct option out_lopt[options_count + 1])
{
    int i_sopt = 0, i_lopt = 0;

    for (int i = 0; i < options_count; i++) {

        /* Short options */
        if (options[i].s_name && options[i].s_name <= UCHAR_MAX) {
            out_sopt[i_sopt++] = options[i].s_name;
            if (options[i].has_arg == required_argument)
                out_sopt[i_sopt++] = ':';
            assert(options[i].has_arg == required_argument ||
                    options[i].has_arg == no_argument);
        }

        /* Long options */
        if (options[i].l_name) {
            assert(options[i].s_name);

            out_lopt[i_lopt].name = options[i].l_name;
            out_lopt[i_lopt].has_arg = options[i].has_arg;
            out_lopt[i_lopt].flag = NULL;
            out_lopt[i_lopt].val = options[i].s_name;

            i_lopt++;
        }
    }

    out_sopt[i_sopt] = '\0';
    memset(&out_lopt[i_lopt], 0, sizeof(struct option));
}

static int config_read_args(int argc, char **argv, char sopt[options_count * 2 + 1],
        struct option lopt[options_count + 1], int level)
{
    int optc, err = NOERR;
    optind = 1;

    while ((optc = getopt_long(argc, argv, sopt,
                    lopt, NULL)) >= 0) {

        bool handled = false;

        for (int i = 0; i < options_count; i++) {
            if (options[i].handle_order != level) {
                /* Lie a lil :x */
                handled = true;
                continue;
            }

            if (optc == options[i].s_name) {
                if (options[i].type != OTYPE_ACTION)
                    err = options[i].set_func(&options[i], optarg);
                else
                    err = options[i].action_func(&options[i]);
                
                if (err != NOERR)
                    goto end;
                options[i].value_is_set = true;

                handled = true;
                break;
            }
        }

        if (handled)
            continue;

        if (optc == '?') {
            err = ERR_OPT_FAILED;
            goto end;
        } else {
            fprintf(stderr, "Unhandled option? '%c'\n", optc);
            err = ERR_OPT_UNHANDLED;
            goto end;
        }
    }

end:
    return err;
}

static enum error config_required_check()
{
    enum error err = NOERR;
    
    for (int i = 0; i < options_count; i++) {
        if (options[i].required && !options[i].value_is_set) {
            printf("Argument %s is required!\n", options[i].l_name);
            err = ERR_OPT_REQUIRED;
        }
    }

    return err;
}

enum error config_parse(int argc, char **argv)
{
    enum error err = NOERR;
    char sopt[options_count * 2 + 1];
    struct option lopt[options_count + 1];
    opt_argv = (const char**)argv;
    opt_argc = argc;
    config_build_getopt_args(sopt, lopt);

    err = config_read_args(argc, argv, sopt, lopt, 0);
    if (err != NOERR)
        goto end;

    err = config_parse_file();
    if (err != NOERR)
        goto end;

    err = config_read_args(argc, argv, sopt, lopt, 1);
    if (err != NOERR)
        goto end;

    /* Set defaults for those, that didn't got set above */
    for (int i = 0; i < options_count; i++) {
        if (!options[i].value_is_set && options[i].type != OTYPE_ACTION && 
                options[i].default_func) {
            err = options[i].default_func(&options[i]);
            if (err != NOERR)
                goto end;
            options[i].value_is_set = true;
        }
    }

    err = config_read_args(argc, argv, sopt, lopt, 2);
    if (err != NOERR)
        goto end;
    
    err = config_required_check();

end:
    if (err != NOERR)
        config_free();
    return err;
}

#if 0
static int config_def_config_dir(struct conf_entry *ce)
{
    char *dir;
    int len;
    const char *format = "%s/.config/" CONFIG_DIR_NAME;
    const char *home_env = getenv("HOME");

    if (!home_env) {
        /* Fix this at a later date with getuid and getpw */
        fprintf(stderr, "HOME environment variable not found!\n");
        return ERR_NOTFOUND;
    }

    len = snprintf(NULL, 0, format, home_env);
    if (len == -1) {
        int err = errno;
        fprintf(stderr, "Failed to call funky snpintf: %s\n", strerror(err));
        return err;
    }
    dir = malloc(len + 1);
    sprintf(dir, format, home_env);

    ce->value.s = dir;
    ce->value_is_dyn = true;

    return NOERR;
}
#endif

#if 0
static int config_def_cachedb(struct conf_entry *ce)
{
    bool dh_free = false;
    const char *data_home = getenv("XDG_DATA_HOME");

    if (!data_home) {
        const char *home = util_get_home();
        if (!home)
            return ERR_OPT_FAILED;
        sprintf(NULL, "%s/.local/share", home);
    }
    return NOERR;
}
#endif

static int config_set_str(struct conf_entry *ce, char *arg)
{
    // TODO use realpath(3), when necessary
    ce->value.s = arg;
    return NOERR;
}

static int config_set_port(struct conf_entry *ce, char *arg)
{
    long portval = strtol(arg, NULL, 10);
    /* A zero return will be invalid no matter if strtol succeeded or not */
    if (portval > UINT16_MAX || portval <= 0) {
        fprintf(stderr, "Invalid port value '%s'\n", arg);
        return ERR_OPT_INVVAL;
    }
    ce->value.hu = (uint16_t)portval;
    return NOERR;
}

static int config_set_bool(struct conf_entry *ce, char *arg)
{
    ce->value.b = true;
    return NOERR;
}

static int show_help(struct conf_entry *ce)
{
    printf("Todo...\n");
    return ERR_OPT_EXIT;
}

static int config_parse_file()
{
    // TODO implement this
#if 0
    assert(conf.config_file_path);
    FILE *f = fopen(conf.config_file_path, "rb");
    char errbuf[200];

    toml_table_t *tml = toml_parse_file(f, errbuf, sizeof(errbuf));
    fclose(f);
    if (!tml) {
        fprintf(stderr, "Failed to parse config toml: %s\n", errbuf);
        return ERR_TOML_PARSE_ERROR;
    }

    toml_datum_t port = toml_int_in(tml, "port");
    if (port.ok)
        conf.port = (uint16_t)port.u.i;
    else
        fprintf(stderr, "Failed to parse port from config toml: %s\n", errbuf);

    toml_datum_t dldir = toml_string_in(tml, "default_download_dir");
    if (dldir.ok) {
        conf.default_download_dir = dldir.u.s;
        printf("%s\n", dldir.u.s);
        conf_dyn.default_download_dir = dldir.u.s;
        conf_dyn.default_download_dir = true;
        /* TODO is this always malloced?? if yes, remve dyn check */
    } else {
        fprintf(stderr, "Failed to parse download dir from config toml: %s\n", errbuf);
    }

    toml_free(tml);
#endif
    return NOERR;
}

int config_free()
{
    for (int i = 0; i < options_count; i++) {
        if (options[i].value_is_dyn) {
            free(options[i].value.s);
            options[i].value.s = NULL;
            options[i].value_is_dyn = false;
            options[i].value_is_set = false;
        }
    }
    return NOERR;
}

#if 0
static int config_action_write_config(struct conf_entry *ce)
{
    /* This is the success return here */
    int err = ERR_OPT_EXIT, plen;
    const char *config_dir;
    FILE *f = NULL;

    config_dir = config_get("config-dir");
    plen = snprintf(NULL, 0, "%s/%s", config_dir, CONFIG_FILE_NAME);
    char path[plen + 1];
    snprintf(path, plen + 1, "%s/%s", config_dir, CONFIG_FILE_NAME);

    for (int i = 0; i < 2; i++) {
        f = fopen(path, "wb");
        if (!f) {
            int errn = errno;
            if (errn == ENOENT && i == 0) {
                /* Try to create parent directory */
                if (mkdir(config_dir, 0755) == -1) {
                    err = errno;
                    fprintf(stderr, "Config mkdir failed: %s\n", strerror(err));
                    goto end;
                }
            } else {
                err = errn;
                fprintf(stderr, "Config fopen failed: %s\n", strerror(err));
                goto end;
            }
        } else {
            break;
        }
    }

    for (int i = 0; i < options_count; i++) {
        if (!options[i].in_file)
            continue;

        fprintf(f, "%s = ", options[i].l_name);
        switch (options[i].type) {
        case OTYPE_S:
            // TODO toml escaping
            fprintf(f, "\"%s\"\n", options[i].value.s);
            break;
        case OTYPE_HU:
            fprintf(f, "%hu\n", options[i].value.hu);
            break;
        case OTYPE_B:
            fprintf(f, "%s\n", options[i].value.b ? "true" : "false");
            break;
        default:
            break;
        }
    }

    config_dump();

end:
    if (f)
        fclose(f);
    return err;
}
#endif

enum error config_get(const char *key, void **out)
{
    enum error err = ERR_OPT_NOTFOUND;

    for (int i = 0; i < options_count; i++) {
        struct conf_entry *cc = &options[i];

        if (strcmp(cc->l_name, key) == 0) {
            if (cc->value_is_set) {
                if (out)
                    *out = &cc->value.s;
                err = NOERR;
            } else {
                err = ERR_OPT_UNSET;
            }
            break;
        }
    }

    return err;
}

const char *config_get_nonopt(int index)
{
    if (index >= config_get_nonopt_count())
        return NULL;
    return opt_argv[optind + index];
}

int config_get_nonopt_count()
{
    return opt_argc - optind;
}

void config_dump()
{
    for (int i = 0; i < options_count; i++) {
        if (options[i].type == OTYPE_ACTION)
            continue;

        printf("%s: ", options[i].l_name);

        if (!options[i].value_is_set) {
            printf("[UNSET (>.<)]\n");
            continue;
        }

        switch (options[i].type) {
        case OTYPE_S:
            printf("%s\n", options[i].value.s);
            break;
        case OTYPE_HU:
            printf("%hu\n", options[i].value.hu);
            break;
        case OTYPE_B:
            printf("%s\n", options[i].value.b ? "True" : "False");
            break;
        default:
            printf("Error :(\n");
            break;
        }
    }
}
