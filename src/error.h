#ifndef _ERROR_H
#define _ERROR_H

#define FE_ERROR(E) \
    E(NOERR = 0) \
    E(ERR_UNKNOWN) \
    E(ERR_NOTFOUND) \
\
    E(ERR_OPT_REQUIRED) \
    E(ERR_OPT_FAILED) \
    E(ERR_OPT_UNHANDLED) \
    E(ERR_OPT_INVVAL) \
    E(ERR_OPT_EXIT) /* We should exit in main, if config_parse returns this */ \
    E(ERR_OPT_UNSET) /* In config_get, if the value isn't set */ \
    E(ERR_OPT_NOTFOUND) /* In config_get, if the options is not found */ \
\
    E(ERR_NET_APIADDR) /* If there are problems with the api servers address */ \
    E(ERR_NET_SOCKET) /* If there are problems with the udp socket */ \
    E(ERR_NET_CONNECTED) /* Socket already connected */ \
    E(ERR_NET_CONNECT_FAIL) /* Connect attempt failed */ \
    E(ERR_NET_NOT_CONNECTED) /* Socket wasn't connected */ \
\
    E(ERR_CMD_FAILED) /* Running the command failed */ \
    E(ERR_CMD_NONE) /* No command was run */ \
    E(ERR_CMD_ARG) /* Some problem with the command arguments */ \
\
    E(ERR_ED2KUTIL_FS) /* Some filesystem problem */ \
    E(ERR_ED2KUTIL_UNSUP) /* Operation or file type is unsupported */ \
    E(ED2KUTIL_DONTHASH) /* Skip the hashing part. pre_hash_fn can return this */ \
\
    E(ERR_API_ENCRYPTFAIL) /* Cannot start encryption with the api */ \
    E(ERR_API_COMMFAIL) /* Communication failure */ \
    E(ERR_API_RESP_INVALID) /* Invalid response */ \
    E(ERR_API_AUTH_FAIL) /* Auth failed */ \
    E(ERR_API_LOGOUT) /* Logout failed */ \
    E(ERR_API_PRINTFFUNC) /* New printf function registration failed */ \
    E(ERR_API_CLOCK) /* Some error with clocks */ \
    E(ERR_API_INVCOMM) /* Invalid command or command arguments */ \
    E(ERR_API_BANNED) /* Got banned */ \
    E(ERR_API_CMD_UNK) /* Unknown command */ \
    E(ERR_API_INT_SRV) /* Internal server error */ \
    E(ERR_API_OOS) /* AniDB is out of service rn */ \
    E(ERR_API_SRV_BUSY) /* Server is too busy, try again later */ \
    E(ERR_API_TIMEOUT) /* Timed out, delay and resubmit */ \
    E(ERR_API_NOLOGIN) /* Login is required for this command */ \
    E(ERR_API_AXX_DENIED) /* Access is denied */ \
    E(ERR_API_INV_SESSION) /* Session is invalid */ \
\
    E(ERR_CACHE_SQLITE) /* Generic sqlite error code */ \
    E(ERR_CACHE_EXISTS) /* Entry already exists, as determined by lid */ \
    /* The entry to be added is not unique, (filename and size duplicate, not hash or lid) */ \
    E(ERR_CACHE_NON_UNIQUE) \
    E(ERR_CACHE_NO_EXISTS) /* Entry does not exists */ \
\
    E(ERR_THRD) /* Generic pthread error */ \
\
    E(ERR_LIBEVENT) /* There are some problem with a libevent function */ \
\
    E(ERR_SHOULD_EXIT) /* Probably got a C-c, program should exit now */ \
    E(_ERR_COUNT) \


#define GEN_ENUM(ENUM) ENUM,
#define GEN_STRING(STRING) #STRING,

enum error {
     FE_ERROR(GEN_ENUM)
};

/*
 * Convert a number (0) to the enum name (NOERR)
 */
const char *error_to_string(enum error err);

#endif /* _ERROR_H */
