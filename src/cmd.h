#ifndef _CMD_H
#define _CMD_H

#include "error.h"
#include "config.h"

/*
 * Read commands from config and execute them
 */
enum error cmd_main();

/*
 * Add files to the AniDB list
 */
enum error cmd_add(void *);
enum error cmd_add_argcheck();

/*
 * Take in a file/folder and print out
 * the ed2k hash of it
 */
enum error cmd_ed2k(void *data);

/*
 * Get and print the server api version
 */
enum error cmd_server_version(void *);

/*
 * Print the server uptime
 */
enum error cmd_server_uptime(void *);

/*
 * Print the program version
 */
enum error cmd_prog_version(void *);

/*
 * Modifies a mylist entry
 */
enum error cmd_modify(void *data);

#endif /* _CMD_H */
