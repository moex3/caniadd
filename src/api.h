#ifndef _API_H
#define _API_H
#include <stdint.h>
#include <time.h>

#include "error.h"

#define API_VERSION "3"

/* Maximum length of one response/request */
#define API_BUFSIZE 1400
/* Session key maximum size, including '\0' */
#define API_SMAXSIZE 16
/* The session timeout in miliseconds */
#define API_TIMEOUT 30 * 60 * 1000

/* How many miliseconds to wait between sends */
#define API_SENDWAIT 2 * 1000
/* The number of packets that are exccempt from the ratelimit */
#define API_FREESEND 5
/* Long term wait between sends */
#define API_SENDWAIT_LONG 4 * 1000
/* After this many packets has been sent, use the longterm ratelimit */
#define API_LONGTERM_PACKETS 100

enum mylist_state {
    MYLIST_STATE_UNKNOWN = 0,
    MYLIST_STATE_INTERNAL,
    MYLIST_STATE_EXTERNAL,
    MYLIST_STATE_DELETED,
    MYLIST_STATE_REMOTE,
};

enum file_state {
    FILE_STATE_NORMAL = 0,
    FILE_STATE_CORRUPT,
    FILE_STATE_SELF_EDIT,
    FILE_STATE_SELF_RIP = 10,
    FILE_STATE_ON_DVD,
    FILE_STATE_ON_VHS,
    FILE_STATE_ON_TV,
    FILE_STATE_IN_THEATERS,
    FILE_STATE_STREAMED,
    FILE_STATE_OTHER = 100,
};

struct api_version_result {
    char version_str[40];
};
struct api_auth_result {
    union {
        char session_key[API_SMAXSIZE];
        /* free() */
        char *banned_reason;
    };
};
struct api_uptime_result {
    int32_t ms;
};
struct api_mylistadd_result {
    union {
        uint64_t new_id;
        struct {
            uint64_t lid, fid, eid, aid, gid, date, viewdate;
            /* free() if != NULL ofc */
            char *storage, *source, *other;
            enum mylist_state state;
            enum file_state filestate;
        };
    };
};

#define e(n) struct api_##n##_result n
struct api_result {
    uint16_t code;
    union {
        struct api_version_result version;
        struct api_auth_result auth;
        struct api_uptime_result uptime;
        e(mylistadd);
    };
};
#undef e

enum error api_init(bool auth);
void api_free();

enum error api_cmd_version(struct api_result *res);
enum error api_cmd_uptime(struct api_result *res);
enum error api_cmd_mylistadd(int64_t size, const uint8_t *hash,
        enum mylist_state fstate, bool watched, struct api_result *res);

#endif /* _API_H */
