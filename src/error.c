#include "error.h"

static const char *error_string[] = {
    FE_ERROR(GEN_STRING)
};

const char *error_to_string(enum error err)
{
    if (err >= _ERR_COUNT)
        return "ERR_UNKNOWN";
    return error_string[err];
}
