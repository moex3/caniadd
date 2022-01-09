#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#include "config.h"
#include "error.h"
#include "uio.h"
#include "cmd.h"
#include "globals.h"

bool should_exit = false;

static void signal_handler(int signum, siginfo_t *info, void *ctx)
{
    should_exit = true;
    printf("\033[0GGot C-c. Press again to force exit\n");
}

int main(int argc, char **argv)
{
    int exit_code = EXIT_SUCCESS;
    enum error err;
    struct sigaction sact = {
        .sa_flags = SA_SIGINFO | SA_RESETHAND,
        //.sa_flags = SA_SIGINFO,
        .sa_sigaction = signal_handler,
    };

    if (sigaction(SIGINT, &sact, NULL) != 0) {
        uio_error("Cannot set up signal handler: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    err = config_parse(argc, argv);
    if (err == ERR_OPT_EXIT)
        return EXIT_SUCCESS;
    else if (err != NOERR)
        return EXIT_FAILURE;

    //config_dump();

    err = cmd_main();
    if (err == ERR_SHOULD_EXIT)
        uio_debug("Exiting as requested orz");
    else if (err != NOERR)
        exit_code = EXIT_FAILURE;

    config_free();

    return exit_code;
}
