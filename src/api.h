#ifndef _API_H
#define _API_H
#include <stdint.h>
#include <time.h>
#include <stdbool.h>

#include "error.h"

#define API_VERSION "3"

/* Maximum length of one response/request */
#define API_BUFSIZE 1400
/* Session key maximum size, including '\0' */
#define API_SMAXSIZE 16
/* Encryption salt maximum size, including '\0' */
#define API_SALTMAXSIZE 16

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

/* How much to wait between try agains? in ms */
#define API_TRYAGAIN_TIME 60 * 1000
#define API_MAX_TRYAGAIN 5

enum api_code {
    APICODE_LOGIN_ACCEPTED                           = 200,
    APICODE_LOGIN_ACCEPTED_NEW_VERSION               = 201,
    APICODE_LOGGED_OUT                               = 203,
    APICODE_RESOURCE                                 = 205,
    APICODE_STATS                                    = 206,
    APICODE_TOP                                      = 207,
    APICODE_UPTIME                                   = 208,
    APICODE_ENCRYPTION_ENABLED                       = 209,
    APICODE_MYLIST_ENTRY_ADDED                       = 210,
    APICODE_MYLIST_ENTRY_DELETED                     = 211,
    APICODE_ADDED_FILE                               = 214,
    APICODE_ADDED_STREAM                             = 215,
    APICODE_EXPORT_QUEUED                            = 217,
    APICODE_EXPORT_CANCELLED                         = 218,
    APICODE_ENCODING_CHANGED                         = 219,
    APICODE_FILE                                     = 220,
    APICODE_MYLIST                                   = 221,
    APICODE_MYLIST_STATS                             = 222,
    APICODE_WISHLIST                                 = 223,
    APICODE_NOTIFICATION                             = 224,
    APICODE_GROUP_STATUS                             = 225,
    APICODE_WISHLIST_ENTRY_ADDED                     = 226,
    APICODE_WISHLIST_ENTRY_DELETED                   = 227,
    APICODE_WISHLIST_ENTRY_UPDATED                   = 228,
    APICODE_MULTIPLE_WISHLIST                        = 229,
    APICODE_ANIME                                    = 230,
    APICODE_ANIME_BEST_MATCH                         = 231,
    APICODE_RANDOM_ANIME                             = 232,
    APICODE_ANIME_DESCRIPTION                        = 233,
    APICODE_REVIEW                                   = 234,
    APICODE_CHARACTER                                = 235,
    APICODE_SONG                                     = 236,
    APICODE_ANIMETAG                                 = 237,
    APICODE_CHARACTERTAG                             = 238,
    APICODE_EPISODE                                  = 240,
    APICODE_UPDATED                                  = 243,
    APICODE_TITLE                                    = 244,
    APICODE_CREATOR                                  = 245,
    APICODE_NOTIFICATION_ENTRY_ADDED                 = 246,
    APICODE_NOTIFICATION_ENTRY_DELETED               = 247,
    APICODE_NOTIFICATION_ENTRY_UPDATE                = 248,
    APICODE_MULTIPLE_NOTIFICATION                    = 249,
    APICODE_GROUP                                    = 250,
    APICODE_CATEGORY                                 = 251,
    APICODE_BUDDY_LIST                               = 253,
    APICODE_BUDDY_STATE                              = 254,
    APICODE_BUDDY_ADDED                              = 255,
    APICODE_BUDDY_DELETED                            = 256,
    APICODE_BUDDY_ACCEPTED                           = 257,
    APICODE_BUDDY_DENIED                             = 258,
    APICODE_VOTED                                    = 260,
    APICODE_VOTE_FOUND                               = 261,
    APICODE_VOTE_UPDATED                             = 262,
    APICODE_VOTE_REVOKED                             = 263,
    APICODE_HOT_ANIME                                = 265,
    APICODE_RANDOM_RECOMMENDATION                    = 266,
    APICODE_RANDOM_SIMILAR                           = 267,
    APICODE_NOTIFICATION_ENABLED                     = 270,
    APICODE_NOTIFYACK_SUCCESSFUL_MESSAGE             = 281,
    APICODE_NOTIFYACK_SUCCESSFUL_NOTIFICATION        = 282,
    APICODE_NOTIFICATION_STATE                       = 290,
    APICODE_NOTIFYLIST                               = 291,
    APICODE_NOTIFYGET_MESSAGE                        = 292,
    APICODE_NOTIFYGET_NOTIFY                         = 293,
    APICODE_SENDMESSAGE_SUCCESSFUL                   = 294,
    APICODE_USER_ID                                  = 295,
    APICODE_CALENDAR                                 = 297,

    APICODE_PONG                                     = 300,
    APICODE_AUTHPONG                                 = 301,
    APICODE_NO_SUCH_RESOURCE                         = 305,
    APICODE_API_PASSWORD_NOT_DEFINED                 = 309,
    APICODE_FILE_ALREADY_IN_MYLIST                   = 310,
    APICODE_MYLIST_ENTRY_EDITED                      = 311,
    APICODE_MULTIPLE_MYLIST_ENTRIES                  = 312,
    APICODE_WATCHED                                  = 313,
    APICODE_SIZE_HASH_EXISTS                         = 314,
    APICODE_INVALID_DATA                             = 315,
    APICODE_STREAMNOID_USED                          = 316,
    APICODE_EXPORT_NO_SUCH_TEMPLATE                  = 317,
    APICODE_EXPORT_ALREADY_IN_QUEUE                  = 318,
    APICODE_EXPORT_NO_EXPORT_QUEUED_OR_IS_PROCESSING = 319,
    APICODE_NO_SUCH_FILE                             = 320,
    APICODE_NO_SUCH_ENTRY                            = 321,
    APICODE_MULTIPLE_FILES_FOUND                     = 322,
    APICODE_NO_SUCH_WISHLIST                         = 323,
    APICODE_NO_SUCH_NOTIFICATION                     = 324,
    APICODE_NO_GROUPS_FOUND                          = 325,
    APICODE_NO_SUCH_ANIME                            = 330,
    APICODE_NO_SUCH_DESCRIPTION                      = 333,
    APICODE_NO_SUCH_REVIEW                           = 334,
    APICODE_NO_SUCH_CHARACTER                        = 335,
    APICODE_NO_SUCH_SONG                             = 336,
    APICODE_NO_SUCH_ANIMETAG                         = 337,
    APICODE_NO_SUCH_CHARACTERTAG                     = 338,
    APICODE_NO_SUCH_EPISODE                          = 340,
    APICODE_NO_SUCH_UPDATES                          = 343,
    APICODE_NO_SUCH_TITLES                           = 344,
    APICODE_NO_SUCH_CREATOR                          = 345,
    APICODE_NO_SUCH_GROUP                            = 350,
    APICODE_NO_SUCH_CATEGORY                         = 351,
    APICODE_BUDDY_ALREADY_ADDED                      = 355,
    APICODE_NO_SUCH_BUDDY                            = 356,
    APICODE_BUDDY_ALREADY_ACCEPTED                   = 357,
    APICODE_BUDDY_ALREADY_DENIED                     = 358,
    APICODE_NO_SUCH_VOTE                             = 360,
    APICODE_INVALID_VOTE_TYPE                        = 361,
    APICODE_INVALID_VOTE_VALUE                       = 362,
    APICODE_PERMVOTE_NOT_ALLOWED                     = 363,
    APICODE_ALREADY_PERMVOTED                        = 364,
    APICODE_HOT_ANIME_EMPTY                          = 365,
    APICODE_RANDOM_RECOMMENDATION_EMPTY              = 366,
    APICODE_RANDOM_SIMILAR_EMPTY                     = 367,
    APICODE_NOTIFICATION_DISABLED                    = 370,
    APICODE_NO_SUCH_ENTRY_MESSAGE                    = 381,
    APICODE_NO_SUCH_ENTRY_NOTIFICATION               = 382,
    APICODE_NO_SUCH_MESSAGE                          = 392,
    APICODE_NO_SUCH_NOTIFY                           = 393,
    APICODE_NO_SUCH_USER                             = 394,
    APICODE_CALENDAR_EMPTY                           = 397,
    APICODE_NO_CHANGES                               = 399,

    APICODE_NOT_LOGGED_IN                            = 403,
    APICODE_NO_SUCH_MYLIST_FILE                      = 410,
    APICODE_NO_SUCH_MYLIST_ENTRY                     = 411,
    APICODE_MYLIST_UNAVAILABLE                       = 412,

    APICODE_LOGIN_FAILED                             = 500,
    APICODE_LOGIN_FIRST                              = 501,
    APICODE_ACCESS_DENIED                            = 502,
    APICODE_CLIENT_VERSION_OUTDATED                  = 503,
    APICODE_CLIENT_BANNED                            = 504,
    APICODE_ILLEGAL_INPUT_OR_ACCESS_DENIED           = 505,
    APICODE_INVALID_SESSION                          = 506,
    APICODE_NO_SUCH_ENCRYPTION_TYPE                  = 509,
    APICODE_ENCODING_NOT_SUPPORTED                   = 519,
    APICODE_BANNED                                   = 555,
    APICODE_UNKNOWN_COMMAND                          = 598,

    APICODE_INTERNAL_SERVER_ERROR                    = 600,
    APICODE_ANIDB_OUT_OF_SERVICE                     = 601,
    APICODE_SERVER_BUSY                              = 602,
    APICODE_NO_DATA                                  = 603,
    APICODE_TIMEOUT                                  = 604,
    APICODE_API_VIOLATION                            = 666,

    APICODE_PUSHACK_CONFIRMED                        = 701,
    APICODE_NO_SUCH_PACKET_PENDING                   = 702,

    APICODE_VERSION                                  = 998,
};

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

struct api_mylistadd_opts {
    enum mylist_state state;
    bool watched;
    uint64_t wdate;
    struct {
        bool state_set : 1;
        bool watched_set : 1;
        bool wdate_set : 1;
    };
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
struct api_encrypt_result {
    char salt[API_SALTMAXSIZE];
};
struct api_uptime_result {
    int32_t ms;
};
struct api_mylist_result {
    uint64_t lid, fid, eid, aid, gid, date, viewdate;
    /* free() if != NULL ofc */
    char *storage, *source, *other;
    enum mylist_state state;
    enum file_state filestate;
};
struct api_mylistadd_result {
    uint64_t new_id;
};
struct api_mylistmod_result {
    uint32_t n_edits;
};
/* The largest struct probably lul 136 bytes */
struct api_myliststats_result {
    uint64_t animes, eps, files, size_of_files;
    uint64_t added_animes, added_eps, added_files;
    uint64_t added_groups, leech_prcnt, glory_prcnt;
    uint64_t viewed_prcnt_of_db, mylist_prcnt_of_db;
    uint64_t viewed_prcnt_of_mylist, num_of_viewed_eps;
    uint64_t votes, reviews, viewed_minutes;
};

#define e(n) struct api_##n##_result n
struct api_result {
    uint16_t code;
    union {
        e(version);
        e(auth);
        e(uptime);
        e(mylist);
        e(mylistadd);
        e(encrypt);
        e(mylistmod);
        e(myliststats);
    };
};
#undef e

enum error api_init(bool auth);
void api_free();

enum error api_cmd_version(struct api_result *res);
enum error api_cmd_uptime(struct api_result *res);
enum error api_cmd_mylist(uint64_t lid, struct api_result *res);
enum error api_cmd_mylistadd(int64_t size, const uint8_t *hash,
        struct api_mylistadd_opts *opts, struct api_result *res);
enum error api_cmd_mylistmod(uint64_t lid, struct api_mylistadd_opts *opts,
        struct api_result *res);
enum error api_cmd_myliststats(struct api_result *res);

#endif /* _API_H */
