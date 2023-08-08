#ifndef _UTIL_H
#define _UTIL_H
#include <sys/stat.h>
#include <time.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "error.h"

#define MS_TO_TIMESPEC(ts, ms) { \
    ts->tv_sec = ms / 1000; \
    ts->tv_nsec = (ms % 1000) * 1000000; \
}

#define MS_TO_TIMESPEC_L(ts, ms) { \
    ts.tv_sec = ms / 1000; \
    ts.tv_nsec = (ms % 1000) * 1000000; \
}

/*
 * Convert bytes to a hex string
 * out needs to be at least (bytes_len * 2 + 1) bytes
 */
void util_byte2hex(const uint8_t* bytes, size_t bytes_len,
        bool uppercase, char* out);

/* And convert it back */
void util_hex2byte(const char *str, uint8_t* out_bytes);
/*
 * Return the user's home directory
 */
const char *util_get_home();

/*
 * Return the filename part of the path
 * This will return a pointer in fullpath
 * !! ONLY WORKS FOR FILES !!
 */
char *util_basename(const char *fullpath);

/*
 * Calculate the difference between 2 timespec structs in miliseconds
 *
 * future cannot be more in the past than past
 * if that makes any sense
 */
uint64_t util_timespec_diff(const struct timespec *past,
        const struct timespec *future);

/*
 * Convert a date and optionally time string into unix time
 * Returns 0 on error
 */
uint64_t util_iso2unix(const char *isotime);

/*
 * Iterate over a given path and call the 'cb' function for each file
 * If 'cb' returns anything other than NOERR, the iteration will stop and
 *  that error will be returned.
 * !! THIS FUNCTION IS NOT THREAD SAFE !!
 */
typedef enum error (*util_itercb)(const char *path, const struct stat *fstat, void *data);
enum error util_iterpath(const char *path, util_itercb cb, void *data);


#endif /* _UTIL_H */
