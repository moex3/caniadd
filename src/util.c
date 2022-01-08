#include <stdlib.h>
#include <string.h>

#include "util.h"

void util_byte2hex(const uint8_t* bytes, size_t bytes_len,
        bool uppercase, char* out)
{
    const char* hex = (uppercase) ? "0123456789ABCDEF" : "0123456789abcdef";
    for (size_t i = 0; i < bytes_len; i++) {
        *out++ = hex[bytes[i] >> 4];
        *out++ = hex[bytes[i] & 0xF];
    }
    *out = '\0';
}

const char *util_get_home()
{
    const char *home_env = getenv("HOME");
    return home_env; /* TODO this can be null, use other methods as fallback */
}

char *util_basename(const char *fullpath)
{
    char *name_part = strrchr(fullpath, '/');
    if (name_part)
        name_part++;
    else
        name_part = (char*)fullpath;
    return name_part;
}

uint64_t util_timespec_diff(const struct timespec *past,
        const struct timespec *future)
{
    int64_t sdiff = future->tv_sec - past->tv_sec;
    int64_t nsdiff = future->tv_nsec - past->tv_nsec; 
    return sdiff * 1000 + (nsdiff / 1000000);
}
