#ifndef _CONFIG_H
#define _CONFIG_H
#include <inttypes.h>
#include <stdbool.h>

#include "error.h"

#ifndef CONFIG_DIR_NAME
#define CONFIG_DIR_NAME "caniadd"
#endif

enum option_type {
    OTYPE_S, /* Stores a string */
    OTYPE_HU, /* Stores an unsigned short */
    OTYPE_B, /* Stores a boolean */
    /* Does not store anything, does an action. Handled after every
     * other option are parsed, and defaults set */
    OTYPE_ACTION, 
    OTYPE_SUBCOMMAND, 

    _OTYPE_COUNT
};

#if 0
/* This should be used to make loopups O(1), but w/e for now */
enum config_global_options {
    OPT_HELP = 0,
    OPT_USERNAME,
    OPT_PASSWORD,
    OPT_PORT,
    OPT_API_SERVER,
    OPT_API_KEY,
    OPT_SAVE_SESSION,
    OPT_DESTROY_SESSION,
    OPT_CACHEDB,
    OPT_DEBUG,
};

enum config_subcommands {
    SUBCOMM_SERVER_VERSION = 0,
    SUBCOMM_VERSION,
    SUBCOMM_UPTIME,
    SUBCOMM_ED2k,
    SUBCOMM_ADD,
    SUBCOMM_MODIFY,
};
#endif

struct conf_entry {
    const char *l_name; /* The long name for the option, or for the config file */
    int s_name; /* Short option name */
    const char *h_desc; /* Description for the help screen */
    union { /* Value of the param */
        char *s;
        uint16_t hu;
        bool b;
    } value;
    /* The function to use to init a default value, if it's a complex one */
    int (*default_func)(struct conf_entry *ce);
    union {
        /* The function to use to set the value of the arg from the
         * command line or from the loaded config file */
        int (*set_func)(struct conf_entry *ce, char *arg);
        /* Callback for an action option */
        int (*action_func)(struct conf_entry *ce);
    };
    int has_arg : 4; /* Do we need to specify an argument for this option on the cmd line? */
    /* Did we set the value? If not, we may need to call the default func */
    bool value_is_set : 1;
    /* Is the value required? */
    bool required : 1;
    bool value_is_dyn : 1; /* Do we need to free the value? */
    bool in_file : 1; /* Is this option in the config file? */
    bool in_args : 1; /* Is this option in the argument list? */
    enum option_type type : 4; /* Type of the option's value */
    /*
     * In which step do we handle this arg?
     * We need this, because:
     * 1. Read in the base dir option from the command line, if present
     * 2. Use the base dir to load the options from the config file
     * 3. Read, and override options from the command line
     * 4. Execute action arguments
     */
    int handle_order : 4;
    /* If type is a subcommand, this is the arguments for that subcommand */
    /* '-h' option is available for all of the subcommands if this is not set */
    int subopts_count;
    struct conf_entry *subopts;
    /* The subcommand parent of this entry, if it is inside of a subcommand */
    //struct conf_entry *subcomm_parent;
};

/*
 * Parse options from the command line
 *
 * Returns 0 on success.
 */
enum error config_parse(int argc, char **argv);

/*
 * Free any memory that may have been allocated
 * in config_parse
 */
int config_free();

/*
 * Write out the options to stdout
 */
void config_dump();

/*
 * Get a config option by its long name
 * The pointer to the options value will be stored, at the pointer pointed to
 *  by out
 * If the option is unset, it returns ERR_OPT_UNSET and out is unchanged
 * It the options is not found, it returns ERR_OPT_NOTFOUND, and out is unchanged
 */
enum error config_get(const char *key, void **out);
enum error config_get_subopt(const char *subcomm, const char *key, void **out);

/*
 * Return an cmd line that is not an option
 * or null if error, or out of elements
 */
const char *config_get_nonopt(int index);
int config_get_nonopt_count();

#endif /* _CONFIG_H */
