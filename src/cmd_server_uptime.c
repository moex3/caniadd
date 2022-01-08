#include <stdio.h>
#include <stdlib.h>

#include "cmd.h"
#include "uio.h"
#include "api.h"

enum error cmd_server_uptime(void *data)
{
    struct api_result r;
    int32_t h, m, s;
    div_t dt;

    if (api_cmd_uptime(&r) != NOERR)
        return ERR_CMD_FAILED;

    if (r.code != 208) {
        uio_error("VERSION cmd is unsuccesful: %hu", r.code);
        return ERR_CMD_FAILED;
    }

    dt = div(r.uptime.ms, 1000*60*60);
    h = dt.quot;
    dt = div(dt.rem, 1000*60);
    m = dt.quot;
    dt = div(dt.rem, 1000);
    s = dt.quot;

    printf("up %d hours, %d minutes, %d seconds\n", h, m, s);
    return NOERR;
}
