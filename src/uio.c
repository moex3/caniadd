#include <stdarg.h>
#include <stdio.h>

#include "uio.h"
#include "config.h"

void uio_user(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);

    vprintf(format, ap);
    printf("\n");

    va_end(ap);
}

void uio_error(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    
    fprintf(stderr, "\033[31m[ERROR]: ");
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\033[0m\n");

    va_end(ap);
}

void uio_debug(const char *format, ...)
{
    bool *dbg_enabled;
    va_list ap;

    config_get("debug", (void**)&dbg_enabled);
    if (!*dbg_enabled)
        return;

    va_start(ap, format);
    
    fprintf(stderr, "\033[35m[DEBUG]: ");
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\033[0m\n");

    va_end(ap);
}

void uio_warning(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    
    printf("\033[33m[WARNING]: ");
    vprintf(format, ap);
    printf("\033[0m\n");

    va_end(ap);
}
