#include <stdio.h>

#include "cmd.h"
#include "cache.h"

#include <string.h>
enum error cmd_prog_version(void *data)
{
    enum error err = NOERR;

    printf("caniadd v0.1.0"
#ifdef GIT_REF
    " (" GIT_REF ")"
#endif
    "\n");

    return err;
}
