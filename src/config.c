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
#include "uio.h"

static int show_help(struct conf_entry *ce);
static int show_subcomm_help(struct conf_entry *ce);
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

#define SUBCOMM_HELP { \
        .l_name = "help", .s_name = 'h', .has_arg = no_argument, \
        .action_func = show_subcomm_help, .in_args = true, \
        .type = OTYPE_ACTION, .handle_order = 0, \
        .h_desc = "Display the help for the subcommand and exit", }


static struct conf_entry subcomm_def_help_opt = SUBCOMM_HELP;

static struct conf_entry ed2k_subopts[] = {
    SUBCOMM_HELP,

    { .l_name = "link", .s_name = 'l', .has_arg = no_argument,
      .set_func = config_set_bool, .in_args = true,
      .type = OTYPE_B, .handle_order = 0, .value_is_set = true,
      .h_desc = "Print an ed2k link for the files", },
};

static struct conf_entry modify_add_subopts[] = {
    SUBCOMM_HELP,

    { .l_name = "watched", .s_name = 'w', .has_arg = no_argument,
      .set_func = config_set_bool, .in_args = true,
      .type = OTYPE_B, .handle_order = 0, .value_is_set = true,
      .h_desc = "Mark the episode as watched when adding files", },

    { .l_name = "wdate", .s_name = UCHAR_MAX + 4, .has_arg = required_argument,
      .set_func = config_set_str, .in_args = true, 
      .type = OTYPE_S, .handle_order = 0,
      .h_desc = "Set the watched date when adding files", },
};

static struct conf_entry options[] = {
    { .l_name = "help", .s_name = 'h', .has_arg = no_argument,
        .action_func = show_help, .in_args = true,
        .type = OTYPE_ACTION, .handle_order = 0,
        .h_desc = "Display the help and exit", },

    { .l_name = "username", .s_name = 'u', .has_arg = required_argument,
      .set_func = config_set_str, .in_args = true, .in_file = true,
      .type = OTYPE_S, .handle_order = 1,
      .h_desc = "Sets the username for the login", },

    { .l_name = "password", .s_name = 'p', .has_arg = required_argument,
      .set_func = config_set_str, .in_args = true, .in_file = true,
      .type = OTYPE_S, .handle_order = 1,
      .h_desc = "Sets the password for the login", },

    { .l_name = "port", .s_name = 'P', .has_arg = required_argument,
      .set_func = config_set_port, .in_args = true, .in_file = true,
      .type = OTYPE_HU, .handle_order = 1, .value.hu = 29937,
      .value_is_set = true,
      .h_desc = "Sets port to use for API server communication", },

    { .l_name = "api-server", .s_name = UCHAR_MAX + 1, .has_arg = required_argument,
      .set_func = config_set_str, .in_args = true, .in_file = true,
      .type = OTYPE_S, .handle_order = 1, .value.s = "api.anidb.net:9000",
      .value_is_set = true,
      .h_desc = "Sets the API server address", },

    { .l_name = "api-key", .s_name = 'k', .has_arg = required_argument,
      .set_func = config_set_str, .in_args = true, .in_file = true,
      .type = OTYPE_S, .handle_order = 1,
      .h_desc = "Sets the api key used for encryption", },

    { .l_name = "save-session", .s_name = 's', .has_arg = no_argument,
      .set_func = config_set_bool, .in_args = true, .in_file = true,
      .type = OTYPE_B, .handle_order = 1, .value_is_set = true,
      .h_desc = "not implemented", },

    { .l_name = "destroy-session", .s_name = 'S', .has_arg = no_argument,
      .set_func = config_set_bool, .in_args = true, .in_file = false,
      .type = OTYPE_B, .handle_order = 1, .value_is_set = true,
      .h_desc = "not implemented", },

    { .l_name = "cachedb", .s_name = 'd', .has_arg = required_argument,
      .set_func = config_set_str, .in_args = true, .in_file = true,
      .type = OTYPE_S, .handle_order = 1, /*.default_func = config_def_cachedb*/
      .h_desc = "Sets the path for the cache database", },

    { .l_name = "debug", .s_name = 'D', .has_arg = no_argument,
      .set_func = config_set_bool, .in_args = true, .in_file = true,
      .type = OTYPE_B, .handle_order = 1, .value_is_set = true,
      .h_desc = "Enable debug output", },

    /*### cmds ###*/

    { .l_name = "server-version", .s_name = UCHAR_MAX + 2,
      .has_arg = no_argument, .set_func = config_set_bool, .in_args = true,
      .type = OTYPE_SUBCOMMAND, .handle_order = 1, .value_is_set = true,
      .h_desc = "Request the server version", 
    },

    { .l_name = "version", .s_name = 'v',
      .has_arg = no_argument, .set_func = config_set_bool, .in_args = true,
      .type = OTYPE_SUBCOMMAND, .handle_order = 1, .value_is_set = true,
      .h_desc = "Print the caniadd version", },

    { .l_name = "uptime", .s_name = UCHAR_MAX + 3,
      .has_arg = no_argument, .set_func = config_set_bool, .in_args = true,
      .type = OTYPE_SUBCOMMAND, .handle_order = 1, .value_is_set = true,
      .h_desc = "Request the uptime of the api servers", },

    { .l_name = "ed2k", .s_name = 'e',
      .has_arg = no_argument, .set_func = config_set_bool, .in_args = true,
      .type = OTYPE_SUBCOMMAND, .handle_order = 1,
      .h_desc = "Run an ed2k hash on the file arguments",
      .subopts = ed2k_subopts,
      .subopts_count = sizeof(ed2k_subopts) / sizeof(ed2k_subopts[0]),
    },

    { .l_name = "add", .s_name = 'a',
      .has_arg = no_argument, .set_func = config_set_bool, .in_args = true,
      .type = OTYPE_SUBCOMMAND, .handle_order = 1,
      .h_desc = "Add files to your anidb list",
      .subopts = modify_add_subopts,
      .subopts_count = sizeof(modify_add_subopts) / sizeof(modify_add_subopts[0]),
    },

    /* Arguments are either mylist id's, or file sizes and names
     *  in the format '[watch_date/]<size>/<filename>'. The filename can't contain
     *  '/' characters.  */
    { .l_name = "modify", .s_name = 'W',
      .has_arg = no_argument, .set_func = config_set_bool, .in_args = true,
      .type = OTYPE_SUBCOMMAND, .handle_order = 1,
      .h_desc = "Modify files in your anidb list",
      .subopts = modify_add_subopts,
      .subopts_count = sizeof(modify_add_subopts) / sizeof(modify_add_subopts[0]),
    },

    /*{ .l_name = "stats", .s_name = UCHAR_MAX + 5,
      .has_arg = no_argument, .set_func = config_set_bool, .in_args = true,
      .type = OTYPE_B, .handle_order = 1, .value_is_set = true },*/

};

static const size_t options_count = sizeof(options) / sizeof(options[0]);
/* Used in show_subcomm_help to output info about the subcommand */
static struct conf_entry *current_subcommand = NULL;
static char **opt_argv = NULL;
static int opt_argc = 0;

static void config_build_getopt_args(int opts_count, const struct conf_entry opts[opts_count],
        char out_sopt[opts_count * 2 + 1], struct option out_lopt[opts_count + 1],
        bool stop_at_1st_nonopt)
{
    int i_sopt = 0, i_lopt = 0;
    if (stop_at_1st_nonopt) {
        /* Tell getopts to stop at the 1st non-option argument */
        out_sopt[i_sopt++] = '+';
    }

    for (int i = 0; i < opts_count; i++) {
        if (opts[i].type == OTYPE_SUBCOMMAND)
            continue;

        /* Short options */
        if (opts[i].s_name && opts[i].s_name <= UCHAR_MAX) {
            out_sopt[i_sopt++] = opts[i].s_name;
            if (opts[i].has_arg == required_argument)
                out_sopt[i_sopt++] = ':';
            assert(opts[i].has_arg == required_argument ||
                    opts[i].has_arg == no_argument);
        }

        /* Long opts */
        if (opts[i].l_name) {
            assert(opts[i].s_name);

            out_lopt[i_lopt].name = opts[i].l_name;
            out_lopt[i_lopt].has_arg = opts[i].has_arg;
            out_lopt[i_lopt].flag = NULL;
            out_lopt[i_lopt].val = opts[i].s_name;

            i_lopt++;
        }
    }

    out_sopt[i_sopt] = '\0';
    memset(&out_lopt[i_lopt], 0, sizeof(struct option));
}

static int config_read_args(int argc, char **argv,
        int opts_count, struct conf_entry opts[opts_count],
        char sopt[opts_count * 2 + 1], struct option lopt[opts_count + 1], int level)
{
    int optc, err = NOERR;
    optind = 1;

    //uio_debug("Before %d %s", argc, argv[0]);
    while ((optc = getopt_long(argc, argv, sopt,
                    lopt, NULL)) >= 0) {

        bool handled = false;
        //uio_debug("Optc: %c", optc);

        for (int i = 0; i < opts_count; i++) {
            if (opts[i].handle_order != level) {
                /* Lie a lil :x */
                handled = true;
                continue;
            }

            if (optc == opts[i].s_name) {
                if (opts[i].type != OTYPE_ACTION)
                    err = opts[i].set_func(&opts[i], optarg);
                else
                    err = opts[i].action_func(&opts[i]);
                
                if (err != NOERR)
                    goto end;
                opts[i].value_is_set = true;

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

static enum error config_parse_subcomm_subopts(struct conf_entry *subcomm)
{
    char sopt[64];
    struct option lopt[32];
    enum error err;
    
    /* Set the global current subcommand pointer */
    current_subcommand = subcomm;
    config_build_getopt_args(subcomm->subopts_count, subcomm->subopts, sopt, lopt, false);

    //uio_debug("Parsing subconn %s", subcomm->l_name);
    //uio_debug("sopt: %s", sopt);

    /* Update args for next parsing and nonopts parsing for later */
    opt_argv = &opt_argv[optind];
    opt_argc = config_get_nonopt_count();
    /* argv[0] (which is the subcommand string) will be treated as the filename here, and skipped */
    err = config_read_args(opt_argc, opt_argv,
            subcomm->subopts_count, subcomm->subopts, sopt, lopt, 0);
    if (err == NOERR) {
        /* Mark subcommand as set */
        subcomm->value_is_set = true;
        subcomm->value.b = true;
    }

    current_subcommand = NULL;
    return err;
}

static enum error config_parse_subcommands()
{
    const char *subcomm_str;
    enum error err = ERR_OPT_NOTFOUND;

    if (config_get_nonopt_count() <= 0) {
        return ERR_OPT_NO_SUBCOMMAND;
    }

    subcomm_str = config_get_nonopt(0);
    for (int i = 0; i < options_count; i++) {
        if (options[i].type != OTYPE_SUBCOMMAND)
            continue;
        if (strcmp(options[i].l_name, subcomm_str) != 0)
            continue;
        if (options[i].subopts == NULL) {
            /* If no suboptions is defined, define the default
             * help one here */
            options[i].subopts = &subcomm_def_help_opt;
            options[i].subopts_count = 1;
        }
        /* Found, parse subopts */
        err = config_parse_subcomm_subopts(&options[i]);
    }
    return err;
}

enum error config_parse(int argc, char **argv)
{
    enum error err = NOERR;
    char sopt[options_count * 2 + 1];
    struct option lopt[options_count + 1];
    opt_argv = argv;
    opt_argc = argc;
    config_build_getopt_args(options_count, options, sopt, lopt, true);

    err = config_read_args(argc, argv, options_count, options, sopt, lopt, 0);
    if (err != NOERR)
        goto end;

    err = config_parse_file();
    if (err != NOERR)
        goto end;

    err = config_read_args(argc, argv, options_count, options, sopt, lopt, 1);
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

    err = config_read_args(argc, argv, options_count, options, sopt, lopt, 2);
    if (err != NOERR)
        goto end;
    
    err = config_required_check();
    if (err != NOERR)
        goto end;

    /* Now that all of the global arguments are parsed, do the subcommands */
    err = config_parse_subcommands();
    if (err != NOERR) {
        if (err == ERR_OPT_NO_SUBCOMMAND)
            printf("No subcommand given!\n\n");
        else if (err == ERR_OPT_NOTFOUND)
            printf("The given subcommand doesn't exist\n\n");
        if (err != ERR_OPT_EXIT) /* If err is this, then help() was already called */
            show_help(NULL);
    }

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

static int show_subcomm_help(struct conf_entry *ce)
{
    assert(current_subcommand);
    const struct conf_entry *subopts = current_subcommand->subopts;

    printf(
            "Usage: caniadd [OPTIONS] %s [SUBCOMMAND_OPTIONS]\n"
            "%s\n"
            "\n"
            "SUBCOMMAND_OPTIONS:\n",
            current_subcommand->l_name,
            current_subcommand->h_desc
    );

    for (size_t i = 0; i < current_subcommand->subopts_count; i++) {
        int printed = 0, pad;

        printed += printf("    ");

        if (subopts[i].l_name)
            printed += printf("--%s", subopts[i].l_name);
        if (subopts[i].s_name < UCHAR_MAX)
            printed += printf(", -%c", subopts[i].s_name);
        if (subopts[i].has_arg != no_argument)
            printed += printf(" arg");

        pad = 25 - printed;
        if (pad <= 0)
            pad = 1;

        printf("%*s%s", pad, "", subopts[i].h_desc);
        if (subopts[i].value_is_set) {
            printf(" Val: ");
            if (subopts[i].type == OTYPE_S)
                printed += printf("%s", subopts[i].value.s);
            else if (subopts[i].type == OTYPE_HU)
                printed += printf("%hu", subopts[i].value.hu);
            else if (subopts[i].type == OTYPE_B)
                printed += printf("%s", subopts[i].value.b ? "true" : "false");
        }
        printf("\n");
    }
    return ERR_OPT_EXIT;
}

static int show_help(struct conf_entry *ce)
{
    printf(
            "Usage: caniadd [OPTIONS] SUBCOMMAND [SUBCOMMAND_OPTIONS]\n"
            "Caniadd will add files to an AniDB list, and possibly more.\n"
            "\n"
            "OPTIONS:\n"
    );
    for (size_t i = 0; i < options_count; i++) {
        if (options[i].type == OTYPE_SUBCOMMAND)
            continue;

        int printed = 0, pad;

        printed += printf("    ");

        if (options[i].l_name)
            printed += printf("--%s", options[i].l_name);
        if (options[i].s_name < UCHAR_MAX)
            printed += printf(", -%c", options[i].s_name);
        if (options[i].has_arg != no_argument)
            printed += printf(" arg");

        pad = 25 - printed;
        if (pad <= 0)
            pad = 1;

        printf("%*s%s", pad, "", options[i].h_desc);
        if (options[i].value_is_set) {
            printf(" Val: ");
            if (options[i].type == OTYPE_S)
                printed += printf("%s", options[i].value.s);
            else if (options[i].type == OTYPE_HU)
                printed += printf("%hu", options[i].value.hu);
            else if (options[i].type == OTYPE_B)
                printed += printf("%s", options[i].value.b ? "true" : "false");
        }
        printf("\n");
    }

    printf("\nSUBCOMMANDS:\n");
    for (size_t i = 0; i < options_count; i++) {
        if (options[i].type != OTYPE_SUBCOMMAND)
            continue;

        int printed = 0, pad;

        printed += printf("    %s", options[i].l_name);

        pad = 25 - printed;
        printf("%*s%s\n", pad, "", options[i].h_desc);
    }
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

enum error config_get_inter(int cenf_count, const struct conf_entry cenf[cenf_count],
        const char *key, void **out)
{
    enum error err = ERR_OPT_NOTFOUND;

    for (int i = 0; i < cenf_count; i++) {
        const struct conf_entry *cc = &cenf[i];

        if (strcmp(cc->l_name, key) == 0) {
            if (cc->value_is_set) {
                if (out)
                    *out = (void**)&cc->value.s;
                err = NOERR;
            } else {
                err = ERR_OPT_UNSET;
            }
            break;
        }
    }

    return err;
}

enum error config_get(const char *key, void **out)
{
    return config_get_inter(options_count, options, key, out);
}

enum error config_get_subopt(const char *subcomm, const char *key, void **out)
{
    for (int i = 0; i < options_count; i++) {
        struct conf_entry *cc = &options[i];

        if (strcmp(cc->l_name, subcomm) == 0) {
            return config_get_inter(cc->subopts_count, cc->subopts, key, out);
        }
    }
    return ERR_OPT_NOTFOUND;
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
