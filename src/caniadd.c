#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "config.h"
#include "error.h"
#include "uio.h"
#include "cmd.h"

int main(int argc, char **argv)
{
    int exit_code = EXIT_SUCCESS;
    enum error err = config_parse(argc, argv);

    if (err == ERR_OPT_EXIT)
        return EXIT_SUCCESS;
    else if (err != NOERR)
        return EXIT_FAILURE;

    //config_dump();

    err = cmd_main();
    if (err != NOERR)
        exit_code = EXIT_FAILURE;

    config_free();

    return exit_code;
}
