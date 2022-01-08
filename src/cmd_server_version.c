#include <stdio.h>

#include "cmd.h"
#include "uio.h"
#include "api.h"

enum error cmd_server_version(void *data)
{
    struct api_result a_res;
    struct api_version_result *vr = &a_res.version;

    if (api_cmd_version(&a_res) != NOERR)
        return ERR_CMD_FAILED;

    if (a_res.code == 998) {
        printf("%s\n", vr->version_str);
    } else {
        uio_error("VERSION cmd is unsuccesful: %hu", a_res.code);
        return ERR_CMD_FAILED;
    }

    return NOERR;
}
