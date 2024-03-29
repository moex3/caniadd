#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <printf.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>

#include <md5.h>
#include <aes.h>

#include "api.h"
#include "net.h"
#include "uio.h"
#include "config.h"
#include "ed2k.h"
#include "util.h"
#include "globals.h"

/* Needed, bcuz of custom %B format */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#ifdef CLOCK_MONOTONIC_COARSE
    #define API_CLOCK CLOCK_MONOTONIC_COARSE
#elif defined(CLOCK_MONOTONIC)
    #warn "No coarse monotonic clock"
    #define API_CLOCK CLOCK_MONOTONIC
#else
    #error "No monotonic clock"
#endif

static enum error api_cmd_base(char buffer[API_BUFSIZE],
        struct api_result *res, const char *fmt, ...)
__attribute__((format (printf, 3, 4)));
static enum error api_cmd_logout(struct api_result *res);
static enum error api_cmd_auth(const char *uname, const char *pass,
        struct api_result *res);
static enum error api_cmd_encrypt(const char *uname, struct api_result *res);

static bool api_authed = false;
static char api_session[API_SMAXSIZE] = {0}; /* No escaping is needed */
static uint8_t e_key[16] = {0};
static bool api_encryption = false;

static pthread_t api_ka_thread = 0;
static pthread_mutex_t api_work_mx;

static struct timespec api_last_packet = {0}; /* Last packet time */
static int32_t api_packet_count = 0; /* Only increment */
//static int32_t api_fast_packet_count = 0; /* Increment or decrement */

/* For some commands, we need a global retry counter */
static int32_t api_g_retry_count = 0;

static int api_escaped_string(FILE *io, const struct printf_info *info,
        const void *const *args)
{
    /* Ignore newline escapes for now */
    char *str = *(char**)args[0];
    char *and_pos = strchr(str, '&');
    size_t w_chars = 0;

    if (and_pos == NULL)
        return fprintf(io, "%s", str);

    while (and_pos) {
        w_chars += fprintf(io, "%.*s", (int)(and_pos - str), str);
        w_chars += fprintf(io, "&amp;");

        str = and_pos + 1;
        and_pos = strchr(str, '&');
    }
    if (*str)
        w_chars += fprintf(io, "%s", str);

    return w_chars;
}

static int api_escaped_sring_info(const struct printf_info *info, size_t n,
        int *argtypes, int *size)
{
    if (n > 0) {
        argtypes[0] = PA_STRING;
        size[0] = sizeof(const char*);
    }
    return 1;
}

static enum error api_init_encrypt(const char *api_key, const char *uname)
{
    MD5Context md5_ctx;
    struct api_result res;
    enum error err;
    size_t salt_len;

    if (api_cmd_encrypt(uname, &res) != NOERR)
        return ERR_API_ENCRYPTFAIL;

    if (res.code != 209) {
        err = ERR_API_ENCRYPTFAIL;
        switch (res.code) {
        case 309:
            uio_error("You'r API key is not defined. Define it here: "
                    "http://anidb.net/perl-bin/animedb.pl?show=profile");
            break;
        case 509:
            uio_error("No such encryption type. Maybe client is outdated?");
            break;
        case 394:
            uio_error("No user with name: '%s' found by AniDB", uname);
            break;
        default:
            uio_error("Unknown encrypt failure: %ld", res.code);
        }
        return err;
    }
    salt_len = strlen(res.encrypt.salt);

    md5Init(&md5_ctx);
    md5Update(&md5_ctx, (uint8_t*)api_key, strlen(api_key));
    md5Update(&md5_ctx, (uint8_t*)res.encrypt.salt, salt_len);
    md5Finalize(&md5_ctx);
    memcpy(e_key, md5_ctx.digest, sizeof(e_key));

#if 1
    char bf[sizeof(e_key) * 2 + 1];
    util_byte2hex(e_key, sizeof(e_key), false, bf);
    uio_debug("Encryption key is: '%s'", bf);
#endif

    api_encryption = true;
    return NOERR;
}

static size_t api_encrypt(char *buffer, size_t data_len)
{
    struct AES_ctx actx;
    size_t rem_data_len = data_len, ret_len = data_len;
    char pad_value;

    AES_init_ctx(&actx, e_key);
    while (rem_data_len >= AES_BLOCKLEN) {
        AES_ECB_encrypt(&actx, (uint8_t*)buffer);

        buffer += AES_BLOCKLEN;
        rem_data_len -= AES_BLOCKLEN;
    }

    /* Possible BOF here? maybe? certanly. */
    pad_value = AES_BLOCKLEN - rem_data_len;
    ret_len += pad_value;

    memset(buffer + rem_data_len, pad_value, pad_value);
    AES_ECB_encrypt(&actx, (uint8_t*)buffer);

    assert(ret_len % AES_BLOCKLEN == 0);
    return ret_len;
}

static size_t api_decrypt(char *buffer, size_t data_len)
{
    assert(data_len % AES_BLOCKLEN == 0);
    
    struct AES_ctx actx;
    size_t ret_len = data_len;
    char pad_value;

    AES_init_ctx(&actx, e_key);
    while (data_len) {
        AES_ECB_decrypt(&actx, (uint8_t*)buffer);
        
        buffer += AES_BLOCKLEN;
        data_len -= AES_BLOCKLEN;
    }

    pad_value = buffer[data_len - 1];
    ret_len -= pad_value;

    return ret_len;
}

static enum error api_auth()
{
    struct api_result res;
    enum error err = NOERR;
    char **uname, **passw;

    if (config_get("username", (void**)&uname) != NOERR) {
        uio_error("Username is not specified, but it is required!");
        return ERR_OPT_REQUIRED;
    }
    if (config_get("password", (void**)&passw) != NOERR) {
        uio_error("Password is not specified, but it is required!");
        return ERR_OPT_REQUIRED;
    }
    /*
     * We could try passing in a session key, if we are executing
     * a login in response to a nologin or inv session error code?
     */

    if (!api_encryption)
        uio_warning("Logging in without encryption!");
    if (api_cmd_auth(*uname, *passw, &res) != NOERR)
        return ERR_API_AUTH_FAIL;

    switch (res.code) {
    case 201:
        uio_warning("A new client version is available!");
    case 200:
        memcpy(api_session, res.auth.session_key, sizeof(api_session));
        api_authed = true;
        uio_debug("Succesfully logged in. Session key: '%s'", api_session);
        break;
    default:
        err = ERR_API_AUTH_FAIL;
        switch (res.code) {
        case 500:
            uio_error("Login failed. Please check your credentials again");
            break;
        case 503:
            uio_error("Client is outdated. You're probably out of luck here.");
            break;
        case 504:
            uio_error("Client is banned :( Reason: %s", res.auth.banned_reason);
            free(res.auth.banned_reason);
            break;
        default:
            uio_error("Unknown error: %hu", res.code);
            break;
        }
    }

    return err;
}

enum error api_logout()
{
    struct api_result res;
    enum error err = NOERR;

    if (api_cmd_logout(&res) != NOERR) {
        return ERR_API_AUTH_FAIL;
    }

    switch (res.code) {
    case 203:
        uio_debug("Succesfully logged out");
        api_authed = false;

        break;
    case 403:
        uio_error("Cannot log out, because we aren't logged in");
        api_authed = false;
        break;
    default:
        err = ERR_API_LOGOUT;
        uio_error("Unknown error: %hu", res.code);
        break;
    }

    return err;
}

static void api_keepalive(struct timespec *out_next)
{
    struct timespec ts = {0};
    uint64_t msdiff;

    clock_gettime(API_CLOCK, &ts);
    msdiff = util_timespec_diff(&api_last_packet, &ts);

    if (msdiff >= API_TIMEOUT) {
        struct api_result r;

        MS_TO_TIMESPEC(out_next, API_TIMEOUT);

        uio_debug("Sending uptime command for keep alive");
        // TODO what if another action is already in progress?
        api_cmd_uptime(&r);
    } else {
        uint64_t msnext = API_TIMEOUT - msdiff;

        uio_debug("Got keepalive request, but time is not up yet");
        MS_TO_TIMESPEC(out_next, msnext);
    }
}

void *api_keepalive_main(void *arg)
{
    struct timespec ka_time;
    MS_TO_TIMESPEC_L(ka_time, API_TIMEOUT);
    uio_debug("Hi from keepalie thread");

    for (;;) {
        if (nanosleep(&ka_time, NULL) != 0) {
            int e = errno;
            uio_error("Nanosleep failed: %s", strerror(e));
        }
        /* Needed, because the thread could be canceled while in recv or send
         * and in that case, the mutex will remain locked
         * Could be replaced with a pthread_cleanup_push ? */
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        pthread_mutex_lock(&api_work_mx);

        uio_debug("G'moooooning! Is it time to keep our special connection alive?");

        api_keepalive(&ka_time);
        uio_debug("Next wakey-wakey in %ld seconds", ka_time.tv_sec);
        pthread_mutex_unlock(&api_work_mx);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    }
    return NULL;
}

enum error api_clock_init()
{
    struct timespec ts;
    memset(&api_last_packet, 0, sizeof(api_last_packet));
    api_packet_count = 0;

    if (clock_getres(API_CLOCK, &ts) != 0) {
        uio_error("Cannot get clock resolution: %s", strerror(errno));
        return ERR_API_CLOCK;
    }
    uio_debug("Clock resolution: %f ms",
            (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000.0));

    return NOERR;
}

enum error api_init(bool auth)
{
    enum error err = NOERR;
    const char **api_key, **uname;

    err = api_clock_init();
    if (err != NOERR)
        return err;

    err = net_init();
    if (err != NOERR)
        return err;

    if (config_get("api-key", (void**)&api_key) == NOERR) {
        if (config_get("username", (void**)&uname) != NOERR) {
            uio_error("Api key is specified, but that also requires "
                    "the username!");
            err = ERR_OPT_REQUIRED;
            goto fail;
        }
        err = api_init_encrypt(*api_key, *uname);
        if (err != NOERR) {
            if (err != ERR_SHOULD_EXIT)
                uio_error("Cannot init api encryption");
            goto fail;
        }
    }

    /* Define an escaped string printf type */
    if (register_printf_specifier('B', api_escaped_string,
                api_escaped_sring_info) != 0) {
        uio_error("Failed to register escaped printf string function");
        err = ERR_API_PRINTFFUNC;
        goto fail;
    }

    if (auth) {
        pthread_mutexattr_t attr;
        int mxres;

        err = api_auth();
        if (err != NOERR)
            goto fail;

        /* Only do keep alive if we have a session */
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        mxres = pthread_mutex_init(&api_work_mx, NULL);
        pthread_mutexattr_destroy(&attr);
        if (mxres != 0) {
            uio_error("Cannot create mutex");
            err = ERR_THRD;
            goto fail;
        }

        if (pthread_create(&api_ka_thread, NULL, api_keepalive_main, NULL) != 0) {
            uio_error("Cannot create api keepalive thread");
            err = ERR_THRD;
            goto fail;
        }

    }

#if 0
    printf("Testings: %B\n", "oi&ha=hi&wooooowz&");
    printf("Testings: %B\n", "oi&ha=hi&wooooowz");
    printf("Testings: %B\n", "&oi&ha=hi&wooooowz");
    printf("Testings: %B\n", "oooooooooiiiiii");
#endif

    return err;
fail:
    api_free();
    return err;
}

void api_free()
{
    if (api_authed) {
        if (pthread_cancel(api_ka_thread) != 0) {
            uio_error("Cannot cancel api keepalive thread");
        } else {
            int je = pthread_join(api_ka_thread, NULL);
            if (je != 0)
                uio_error("Cannot join api keepalive thread: %s",
                        strerror(je));
            else
                uio_debug("Keepalive thread ended");

            if (pthread_mutex_destroy(&api_work_mx) != 0)
                uio_error("Cannot destroy api work mutex");
        }

        api_logout();
        memset(api_session, 0, sizeof(api_session));
        api_authed = false; /* duplicate */
    }
    if (api_encryption) {
        api_encryption = false;
        memset(e_key, 0, sizeof(e_key));
    }

    register_printf_specifier('B', NULL, NULL);
    net_free();
}

/*
 * We just sent a packet, so update the last packet time here
 */
static void api_ratelimit_sent()
{
    api_packet_count++;
    clock_gettime(API_CLOCK, &api_last_packet);
}

static enum error api_ratelimit()
{
    struct timespec ts = {0};
    uint64_t msdiff, mswait;
    uint64_t msrate = api_packet_count >= API_LONGTERM_PACKETS ?
        API_SENDWAIT_LONG : API_SENDWAIT;

    /* First of all, the first N packets are unmetered */
    if (api_packet_count <= API_FREESEND) {
        uio_debug("This packet is for free! Yay :D (%d/%d)",
                api_packet_count, API_FREESEND);
        return NOERR;
    }

    clock_gettime(API_CLOCK, &ts);
    msdiff = util_timespec_diff(&api_last_packet, &ts);
    uio_debug("Time since last packet: %ld ms", msdiff);

    if (msdiff >= msrate)
        return NOERR; /* No ratelimiting is needed */

    /* Need ratelimit, so do it here for now */
    mswait = msrate - msdiff;
    uio_debug("Ratelimit is needed, sleeping for %ld ms", mswait);

    MS_TO_TIMESPEC_L(ts, mswait);
    if (nanosleep(&ts, NULL) == -1) {
        if (errno == EINTR && should_exit)
            return ERR_SHOULD_EXIT;
        else
            uio_error("Nanosleep failed: %s", strerror(errno));
    }

    return NOERR;
}

/*
 * Returns the written byte count
 * Or negative errno value on error
 */
static ssize_t api_send(char *buffer, size_t data_len, size_t buf_size)
{
    pthread_mutex_lock(&api_work_mx);

    ssize_t read_len;
    int en;

    if (api_ratelimit() == ERR_SHOULD_EXIT) {
        read_len = -2;
        goto end;
    }
    uio_debug("{Api}: Sending: %.*s", (int)data_len, buffer);
    if (api_encryption)
        data_len = api_encrypt(buffer, data_len);

    en = net_send(buffer, data_len);
    if (en < 0) {
        read_len = en;
        goto end;
    }

    read_len = net_read(buffer, buf_size);
    if (read_len < 0) {
        if (read_len == -EINTR)
            uio_error("!!! BAD PLACE EINTR !!!        report pls");
        goto end;        /* This could lead so some problems if we also want to
                            log out. If we hit this, the msg got sent, but we
                            couldn't read the response. That means, in the
                            logout call, this msg's data will be read
                            Let's see if this ever comes up */
    }
    api_ratelimit_sent();

    if (api_encryption)
        read_len = api_decrypt(buffer, read_len);
    uio_debug("{Api}: Reading: %.*s", (int)read_len, buffer);

end:
    pthread_mutex_unlock(&api_work_mx);
    return read_len;
}

long api_res_code(const char *buffer)
{
    char *end;
    long res = strtol(buffer, &end, 10);
    if (res == 0 && buffer == end) {
        uio_error("No error codes in the response");
        return -1;
    }
    assert(*end == ' ');
    return res;
}

static bool api_get_fl(const char *buffer, int32_t index, const char *delim,
        char **const out_start, size_t *const out_len)
{
    assert(index > 0);

    size_t len = strcspn(buffer, delim);

    while (--index > 0) {
        buffer += len + 1;
        len = strcspn(buffer, delim);
    }

    *out_start = (char*)buffer;
    *out_len = len;
    return true;
}

static bool api_get_line(const char *buffer, int32_t line_num,
        char **const out_line_start, size_t *const out_line_len)
{
    return api_get_fl(buffer, line_num, "\n", out_line_start, out_line_len);
}

static bool api_get_field(const char *buffer, int32_t field_num,
        char **const out_field_start, size_t *const out_field_len)
{
    return api_get_fl(buffer, field_num, " |\n", out_field_start, out_field_len);
}

/*
 * Parse a line, where fields are separated by '|'
 * Varargs should be:
 *  A scanf like format specifier - out ptr, ...
 *  Ended with a NULL format specifier
 *
 * %s is overridden, so if the string is 0 length, it will return NULL
 *
 * Returns the number of fields parsed
 */
int api_field_parse(const char *line, ...)
{
    int f_count = 1;
    va_list ap;
    const char *fmt;

    va_start(ap, line);
    fmt = va_arg(ap, const char*);
    while (fmt) {
        char *fs;
        size_t fl;
        void *ptr;
        char scan_fmt[32];

        assert(fmt[0] == '%');
        api_get_fl(line, f_count, "|\n", &fs, &fl);
        assert(!(fl == 0 && fmt[1] != 's'));
        ptr = va_arg(ap, void*);
        if (fl == 0) {
            *(void**)ptr = NULL;
            goto next;
        }
        if (fmt[1] == 's') {
            *(char**)ptr = strndup(fs, fl);
            goto next;
        }
        snprintf(scan_fmt, sizeof(scan_fmt), "%%%d%s", fl, &fmt[1]);
        if (sscanf(fs, scan_fmt, ptr) != 1)
            goto end;
next:
        f_count++;
        fmt = va_arg(ap, const char*);
    }

end:
    va_end(ap);
    return f_count - 1;
}

#if 0
static char *api_get_field_mod(char *buffer, int32_t field_num)
{
    char *sptr = NULL;
    char *f_start;

    f_start = strtok_r(buffer, " ", &sptr);
    if (!f_start)
        return NULL;

    while (field_num --> 0) {
        f_start = strtok_r(NULL, " ", &sptr);
        if (!f_start)
            return NULL;
    }

    return f_start;
}
#endif

/* Basically convert remote codes into local error codes */
static enum error api_cmd_base_errorc(long code, char buffer[API_BUFSIZE])
{
    switch (code) {
    case APICODE_ILLEGAL_INPUT_OR_ACCESS_DENIED:
        uio_error("Got unretryable error code: %d", code);
        return ERR_API_INVCOMM;
    case APICODE_BANNED:
        {
            char *ls;
            size_t ll;

            api_get_line(buffer, 2, &ls, &ll);
            uio_error("Banned: %.*s", (int)ll, ls);
            return ERR_API_BANNED;
        }
    case APICODE_UNKNOWN_COMMAND:
        uio_error("The sent command is unknown");
        return ERR_API_CMD_UNK;
    case APICODE_INTERNAL_SERVER_ERROR:
        uio_error("Internal server error!");
        return ERR_API_INT_SRV;
    case APICODE_ANIDB_OUT_OF_SERVICE:
        uio_error("AniDB is currently out of service");
        return ERR_API_OOS;
    case APICODE_SERVER_BUSY:
        uio_warning("Server is busy rn, trying again later");
        return ERR_API_SRV_BUSY;
    case APICODE_TIMEOUT:
        uio_debug("Timed out, retrying");
        return ERR_API_TIMEOUT;
    case APICODE_LOGIN_FIRST:
        uio_error("This command required AUTH");
        return ERR_API_NOLOGIN;
    case APICODE_ACCESS_DENIED:
        uio_error("Access is denied for this info");
        return ERR_API_AXX_DENIED;
    case APICODE_INVALID_SESSION:
        uio_error("The login session is invalid");
        return ERR_API_INV_SESSION;
    default:
        /* Not an error, or at least not a base error */
        return NOERR;
    }
}

/*
 * Base for all api_cmd's. This will also execute the default
 * error code handers, like 505, 555, 604...
 * If success, res.code will be filled out
 */
static enum error api_cmd_base_pref(char buffer[API_BUFSIZE], int send_len,
        struct api_result *res)
{
    enum error err = NOERR;
    int retry_count = 0;

    api_g_retry_count = 0;

    while (retry_count < API_MAX_TRYAGAIN &&
            api_g_retry_count < API_MAX_TRYAGAIN) {
        long code;
        char l_buff[API_BUFSIZE];
        ssize_t res_len;

        /* Need the copy because the response will be written here too,
         * and we will send the response back when retrying */
        memcpy(l_buff, buffer, send_len);
        res_len = api_send(buffer, send_len, API_BUFSIZE);

        if (res_len < 0) {
            if (res_len == -EINTR && should_exit)
                err = ERR_SHOULD_EXIT;
            else
                err = ERR_API_COMMFAIL;
            goto end;
        }

        code = api_res_code(buffer);
        if (code == -1) {
            err = ERR_API_RESP_INVALID;
            goto end;
        }
        res->code = code;

        err = api_cmd_base_errorc(code, buffer);
        if (err == ERR_API_OOS || err == ERR_API_SRV_BUSY ||
                err == ERR_API_TIMEOUT) {
            struct timespec ts;
            time_t wait_ms = API_TRYAGAIN_TIME;

            if (err == ERR_API_OOS) {
                /* In this case, we should wait 30 minutes as per the wiki */
                wait_ms = 30 * 60 * 1000;
            }

            MS_TO_TIMESPEC_L(ts, wait_ms);
            retry_count++;
            uio_debug("Retry after %ld ms (%d/%d)", wait_ms,
                    retry_count, API_MAX_TRYAGAIN);
            if (nanosleep(&ts, NULL) == -1) {
                if (errno == EINTR && should_exit) {
                    err = ERR_SHOULD_EXIT;
                    goto end;
                }
            }
            /* Copy the saved command back */
            memcpy(buffer, l_buff, send_len);
            continue;
        }

        if (err == ERR_API_NOLOGIN || err == ERR_API_INV_SESSION) {
            if (api_authed == false) {
                /* If this happens and we are not logged in, don't retry */
                err = ERR_API_COMMFAIL;
                break;
            }
            api_g_retry_count++;
            if (api_g_retry_count < API_MAX_TRYAGAIN) {
                struct timespec ts;
                MS_TO_TIMESPEC_L(ts, 30000);

                uio_debug("Let's try loggin in agane after waiting some");
                api_authed = false; /* We got logged out probably */
                if (nanosleep(&ts, NULL) == -1) {
                    if (errno == EINTR && should_exit) {
                        err = ERR_SHOULD_EXIT;
                        goto end;
                    }
                }
                err = api_auth(); /* -> will call this function */
                if (api_g_retry_count < API_MAX_TRYAGAIN) {
                    memcpy(buffer, l_buff, send_len);
                    continue;
                }
            }
            break;
        }

        break;
    };
    if (retry_count >= API_MAX_TRYAGAIN ||
            api_g_retry_count >= API_MAX_TRYAGAIN) {
        uio_debug("Max retry count reached");
        goto end;
    }

end:
    return err;
}
static enum error api_cmd_base(char buffer[API_BUFSIZE], struct api_result *res,
        const char *fmt, ...)
{
    va_list ap;
    int send_len;

    va_start(ap, fmt);
    send_len = vsnprintf(buffer, API_BUFSIZE, fmt, ap);
    va_end(ap);

    return api_cmd_base_pref(buffer, send_len, res);
}

static enum error api_cmd_encrypt(const char *uname, struct api_result *res)
{
    char buffer[API_BUFSIZE];
    enum error err;

    /* Usernames can't contain '&' */
    err = api_cmd_base(buffer, res, "ENCRYPT user=%s&type=1", uname);
    if (err != NOERR)
        return err;

    if (res->code == APICODE_ENCRYPTION_ENABLED) {
        char *fs;
        size_t fl;
        bool gfl = api_get_field(buffer, 2, &fs, &fl);

        assert(gfl);
        (void)gfl;
        assert(sizeof(res->encrypt.salt) > fl);
        memcpy(res->encrypt.salt, fs, fl);
        res->encrypt.salt[fl] = '\0';
    }

    return err;
}

enum error api_cmd_version(struct api_result *res)
{
    char buffer[API_BUFSIZE];
    enum error err;

    err = api_cmd_base(buffer, res, "VERSION");
    if (err != NOERR)
        return err;

    if (res->code == APICODE_VERSION) {
        char *ver_start;
        size_t ver_len;
        bool glr = api_get_line(buffer, 2, &ver_start, &ver_len);

        assert(glr);
        (void)glr;
        assert(ver_len < sizeof(res->version.version_str));
        memcpy(res->version.version_str, ver_start, ver_len);
        res->version.version_str[ver_len] = '\0';
    }
    
    return err;
}

static enum error api_cmd_auth(const char *uname, const char *pass,
        struct api_result *res)
{
    char buffer[API_BUFSIZE];
    enum error err;

    err = api_cmd_base(buffer, res, "AUTH user=%s&pass=%B&protover="
            API_VERSION "&client=caniadd&clientver=" PROG_VERSION
            "&enc=UTF-8", uname, pass);
    if (err != NOERR)
        return err;

    if (res->code == APICODE_LOGIN_ACCEPTED ||
            res->code == APICODE_LOGIN_ACCEPTED_NEW_VERSION) {
        char *sess;
        size_t sess_len;
        bool gfr = api_get_field(buffer, 2, &sess, &sess_len);

        assert(gfr);
        (void)gfr;
        assert(sess_len < sizeof(res->auth.session_key));
        memcpy(res->auth.session_key, sess, sess_len);
        res->auth.session_key[sess_len] = '\0';
    } else if (res->code == APICODE_CLIENT_BANNED) {
        char *reason;
        size_t reason_len;
        bool gfr = api_get_field(buffer, 5, &reason, &reason_len);

        assert(gfr);
        (void)gfr;
        res->auth.banned_reason = strndup(reason, reason_len);
    }

    return err;
}

static enum error api_cmd_logout(struct api_result *res)
{
    char buffer[API_BUFSIZE];
    enum error err;

    err = api_cmd_base(buffer, res, "LOGOUT s=%s", api_session);
    if (err != NOERR)
        return err;

    return err;
}

enum error api_cmd_uptime(struct api_result *res)
{
    char buffer[API_BUFSIZE];
    enum error err;

    err = api_cmd_base(buffer, res, "UPTIME s=%s", api_session);
    if (err != NOERR)
        return err;

    if (res->code == APICODE_UPTIME) {
        char *ls;
        size_t ll;
        bool glf = api_get_line(buffer, 2, &ls, &ll);

        assert(glf);
        (void)glf;
        res->uptime.ms = strtol(ls, NULL, 10);
    }

    return err;
}

static enum error api_cmd_mylist_resp_parse(const char *buffer,
        struct api_mylist_result *mr)
{
    /* {int4 lid}|{int4 fid}|{int4 eid}|{int4 aid}|{int4 gid}|
     * {int4 date}|{int2 state}|{int4 viewdate}|{str storage}|
     * {str source}|{str other}|{int2 filestate} */

    int fc; /* the Freedom Club */
    char *ls;
    size_t ll;
    enum error err = NOERR;
    bool glr = api_get_line(buffer, 2, &ls, &ll);
    assert(glr);
    assert(ll < API_BUFSIZE - 1);
    (void)glr;

    fc = api_field_parse(ls,
            "%Lu", &mr->lid, "%Lu", &mr->fid, "%Lu", &mr->eid,
            "%Lu", &mr->aid, "%Lu", &mr->gid, "%Lu", &mr->date,
            "%hu", &mr->state, "%Lu", &mr->viewdate, "%s", &mr->storage,
            "%s", &mr->source, "%s", &mr->other, "%hu", &mr->filestate,
            NULL);

    uio_debug("Fc is: %d", fc);
    if (fc != 12) {
        if (fc >= 9)
            free(mr->storage);
        if (fc >= 10)
            free(mr->source);
        if (fc >= 11)
            free(mr->other);
        uio_error("Scanf only parsed %d", fc);
        err = ERR_API_RESP_INVALID;
    }

    return err;
}

enum error api_cmd_mylistadd(int64_t size, const uint8_t *hash,
        struct api_mylistadd_opts *opts, struct api_result *res)
{
    char buffer[API_BUFSIZE];
    char hash_str[ED2K_HASH_SIZE * 2 + 1];
    enum error err = NOERR;
    int send_len;

    util_byte2hex(hash, ED2K_HASH_SIZE, false, hash_str);
    /* Wiki says file size is 4 bytes, but no way that's true lol */
    send_len = snprintf(buffer, sizeof(buffer),
            "MYLISTADD s=%s&size=%ld&ed2k=%s",
            api_session, size, hash_str);
    if (opts->state_set)
        send_len += snprintf(buffer + send_len, sizeof(buffer) - send_len,
                "&state=%hu", opts->state);
    if (opts->watched_set)
        send_len += snprintf(buffer + send_len, sizeof(buffer) - send_len,
                "&viewed=%d", opts->watched);
    if (opts->wdate_set)
        send_len += snprintf(buffer + send_len, sizeof(buffer) - send_len,
                "&viewdate=%ld", opts->wdate);
    err = api_cmd_base_pref(buffer, send_len, res);
    if (err != NOERR)
        return err;

    if (res->code == APICODE_MYLIST_ENTRY_ADDED) {
        char *ls, id_str[12];
        size_t ll;
        bool glr = api_get_line(buffer, 2, &ls, &ll);

        assert(glr);
        (void)glr;
        assert(sizeof(id_str) > ll);
        memcpy(id_str, ls, ll);
        id_str[ll] = '\0';
        res->mylistadd.new_id = strtoll(id_str, NULL, 10);
        /* Wiki says these id's are 4 bytes, which is untrue...
         * that page may be a little out of date (or they just
         * expect us to use common sense lmao */
    } else if (res->code == APICODE_FILE_ALREADY_IN_MYLIST) {
        err = api_cmd_mylist_resp_parse(buffer, &res->mylist);
    }

    return err;
}

enum error api_cmd_mylist(uint64_t lid, struct api_result *res)
{
    char buffer[API_BUFSIZE];
    enum error err = NOERR;
    
    err = api_cmd_base(buffer, res, "MYLIST s=%s&lid=%lu", api_session, lid);
    if (err != NOERR)
        return err;

    if (res->code == APICODE_MYLIST)
        err = api_cmd_mylist_resp_parse(buffer, &res->mylist);

    return err;
}

enum error api_cmd_mylistmod(uint64_t lid, struct api_mylistadd_opts *opts,
        struct api_result *res)
{
    char buffer[API_BUFSIZE];
    enum error err = NOERR;
    int send_len;

    send_len = snprintf(buffer, sizeof(buffer),
            "MYLISTADD s=%s&lid=%lu&edit=1", api_session, lid);
    if (opts->state_set)
        send_len += snprintf(buffer + send_len, sizeof(buffer) - send_len,
                "&state=%hu", opts->state);
    if (opts->watched_set)
        send_len += snprintf(buffer + send_len, sizeof(buffer) - send_len,
                "&viewed=%d", opts->watched);
    if (opts->wdate_set)
        send_len += snprintf(buffer + send_len, sizeof(buffer) - send_len,
                "&viewdate=%ld", opts->wdate);
    err = api_cmd_base_pref(buffer, send_len, res);

    return err;
}

static enum error api_cmd_myliststats_resp_parse(const char *buffer, struct api_myliststats_result *stats)
{
    /* all int
     *
     * {animes}|{eps}|{files}|{size of files}|{added animes}|{added eps}|
     * {added files}|{added groups}|{leech %}|{glory %}|{viewed % of db}|
     * {mylist % of db}|{viewed % of mylist}|{number of viewed eps}|
     * {votes}|{reviews}|{viewed length in minutes}
     */

    bool glr;
    char *line;
    size_t line_len;
    int fc;

    glr = api_get_line(buffer, 2, &line, &line_len);
    if (!glr)
        return ERR_API_RESP_INVALID;
    fc = api_field_parse(line,
            "%lu", &stats->animes,
            "%lu", &stats->eps,
            "%lu", &stats->files,
            "%lu", &stats->size_of_files,
            "%lu", &stats->added_animes,
            "%lu", &stats->added_eps,
            "%lu", &stats->added_files,
            "%lu", &stats->added_groups,
            "%lu", &stats->leech_prcnt,
            "%lu", &stats->glory_prcnt,
            "%lu", &stats->viewed_prcnt_of_db,
            "%lu", &stats->mylist_prcnt_of_db,
            "%lu", &stats->viewed_prcnt_of_mylist,
            "%lu", &stats->num_of_viewed_eps,
            "%lu", &stats->votes,
            "%lu", &stats->reviews,
            "%lu", &stats->viewed_minutes,
            NULL);
    if (fc != 17) {
        uio_error("Scanf only parsed %d", fc);
        return ERR_API_RESP_INVALID;
    }

    return NOERR;
}

enum error api_cmd_myliststats(struct api_result *res)
{
    char buffer[API_BUFSIZE];
    enum error err = NOERR;

    err = api_cmd_base(buffer, res, "MYLISTSTATS s=%s", api_session);
    //err = api_cmd_base(buffer, res, "MYLISTSTATS");
    if (err != NOERR)
        return err;

    if (res->code == APICODE_MYLIST_STATS)
        return api_cmd_myliststats_resp_parse(buffer, &res->myliststats);
    else
        return ERR_API_RESP_INVALID;
}

#pragma GCC diagnostic pop
