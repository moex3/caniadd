#include <stddef.h>

#include <sqlite3.h>

#include "cache.h"
#include "config.h"
#include "uio.h"
#include "ed2k.h"
#include "util.h"

static bool cache_did_init = false;

#define sqlite_bind_goto(smt, name, type, ...) { \
        int sb_idx = sqlite3_bind_parameter_index(smt, name); \
        if (sb_idx == 0) { \
            uio_error("Cannot get named parameter for var: %s", name); \
            err = ERR_CACHE_SQLITE; \
            goto fail; \
        } \
        int sb_sret = sqlite3_bind_##type(smt, sb_idx, __VA_ARGS__); \
        if (sb_sret != SQLITE_OK) {\
            uio_error("Cannot bind to statement: %s", sqlite3_errmsg(cache_db));\
            err = ERR_CACHE_SQLITE;\
            goto fail;\
        } \
    }

static sqlite3 *cache_db = NULL;

static const char sql_create_table[] = "CREATE TABLE IF NOT EXISTS mylist ("
        "lid INTEGER NOT NULL PRIMARY KEY,"
        "fname TEXT NOT NULL,"
        "fsize INTEGER NOT NULL,"
        "ed2k TEXT NOT NULL,"
        "watchdate INTEGER,"
        "state INTEGER NOT NULL,"
        "moddate INTEGER NOT NULL DEFAULT (strftime('%s')),"
        "UNIQUE (fname, fsize) )";
static const char sql_mylist_add[] = "INSERT INTO mylist "
        "(lid, fname, fsize, ed2k, watchdate, state) VALUES "
        "(:lid, :fname, :fsize, :ed2k, :watchdate, :state)";
static const char sql_mylist_get[] = "SELECT * FROM mylist WHERE "
        "fsize=:fsize AND fname=:fname";


#if 0
static const char sql_has_tables[] = "SELECT 1 FROM sqlite_master "
        "WHERE type='table' AND tbl_name='mylist'";

/* Return 0 if false, 1 if true, and -1 if error */
static int cache_has_tables()
{
    sqlite3_smt smt;
    int sret;

    sret = sqlite3_prepare_v2(cache_db, sql_has_tables,
            sizeof(sql_has_tables), &smt, NULL);
    if (sret != SQLITE_OK) {
        uio_error("Cannot prepare statement: %s", sqlite3_errmsg(cache_db));
        return -1;
    }

    sqlite3_step(&smt);
    // ehh fuck this, lets just use if not exists
    
    sret = sqlite3_finalize(&smt);
    if (sret != SQLITE_OK)
        uio_debug("sql3_finalize failed: %s", sqlite3_errmsg(cache_db));
}
#endif

/*
 * Create database table(s)
 */
static enum error cache_init_table()
{
    sqlite3_stmt *smt;
    int sret;
    enum error err = NOERR;

    sret = sqlite3_prepare_v2(cache_db, sql_create_table,
            sizeof(sql_create_table), &smt, NULL);
    if (sret != SQLITE_OK) {
        uio_error("Cannot prepare statement: %s", sqlite3_errmsg(cache_db));
        return ERR_CACHE_SQLITE;
    }

    sret = sqlite3_step(smt);
    if (sret != SQLITE_DONE) {
        uio_error("sql3_step is not done: %s", sqlite3_errmsg(cache_db));
        err = ERR_CACHE_SQLITE;
    }
    
    sret = sqlite3_finalize(smt);
    if (sret != SQLITE_OK)
        uio_debug("sql3_finalize failed: %s", sqlite3_errmsg(cache_db));

    return err;
}

enum error cache_init()
{
    char **db_path;
    enum error err;
    int sret;

    err = config_get("cachedb", (void**)&db_path);
    if (err != NOERR) {
        uio_error("Cannot get cache db path from args");
        return err;
    }

    uio_debug("Opening cache db: '%s'", *db_path);
    sret = sqlite3_open(*db_path, &cache_db);
    if (sret != SQLITE_OK) {
        uio_error("Cannot create sqlite3 database: %s", sqlite3_errstr(sret));
        sqlite3_close(cache_db); /* Even if arg is NULL, it's A'OK */
        return ERR_CACHE_SQLITE;
    }
    sqlite3_extended_result_codes(cache_db, 1);

    err = cache_init_table();
    if (err != NOERR)
        goto fail;

    cache_did_init = true;
    return NOERR;

fail:
    cache_free();
    return err;
}

bool cache_is_init()
{
    return cache_did_init;
}

void cache_free()
{
    cache_did_init = false;
    sqlite3_close(cache_db);
    uio_debug("Closed cache db");
}

enum error cache_add(uint64_t lid, const char *fname,
        uint64_t fsize, const uint8_t *ed2k, uint64_t watchdate,
        enum mylist_state state)
{
    char ed2k_str[ED2K_HASH_SIZE * 2 + 1];
    sqlite3_stmt *smt;
    int sret;
    enum error err = NOERR;

    sret = sqlite3_prepare_v2(cache_db, sql_mylist_add,
            sizeof(sql_mylist_add), &smt, NULL);
    if (sret != SQLITE_OK) {
        uio_error("Cannot prepare statement: %s", sqlite3_errmsg(cache_db));
        return ERR_CACHE_SQLITE;
    }

    util_byte2hex(ed2k, ED2K_HASH_SIZE, false, ed2k_str);

    sqlite_bind_goto(smt, ":lid", int64, lid);
    sqlite_bind_goto(smt, ":fname", text, fname, -1, SQLITE_STATIC);
    sqlite_bind_goto(smt, ":fsize", int64, fsize);
    sqlite_bind_goto(smt, ":ed2k", text, ed2k_str, -1, SQLITE_STATIC);
    sqlite_bind_goto(smt, ":watchdate", int64, watchdate);
    sqlite_bind_goto(smt, ":state", int64, state);

    sret = sqlite3_step(smt);
    if (sret != SQLITE_DONE) {
        if (sret == SQLITE_CONSTRAINT_PRIMARYKEY) {
            uio_debug("Attempted to add duplicate entry!");
            err = ERR_CACHE_EXISTS;
        } else if (sret == SQLITE_CONSTRAINT_UNIQUE) {
            uio_debug("An entry with the same name and size already exists!");
            err = ERR_CACHE_NON_UNIQUE;
        } else {
            uio_error("error after sql3_step: %s %d", sqlite3_errmsg(cache_db), sret);
            err = ERR_CACHE_SQLITE;
        }
    }
    
fail:
    sqlite3_finalize(smt);
    return err;
    
}

enum error cache_get(const char *fname, uint64_t fsize, enum cache_select sel,
        struct cache_entry *out_ce)
{
    sqlite3_stmt *smt;
    int sret;
    enum error err = NOERR;

    sret = sqlite3_prepare_v2(cache_db, sql_mylist_get,
            sizeof(sql_mylist_get), &smt, NULL);
    if (sret != SQLITE_OK) {
        uio_error("Cannot prepare statement: %s", sqlite3_errmsg(cache_db));
        return ERR_CACHE_SQLITE;
    }

    sqlite_bind_goto(smt, ":fname", text, fname, -1, SQLITE_STATIC);
    sqlite_bind_goto(smt, ":fsize", int64, fsize);

    sret = sqlite3_step(smt);
    if (sret == SQLITE_DONE) {
        uio_debug("Cache entry with size (%lu) and name (%s) not found", fsize, fname);
        err = ERR_CACHE_NO_EXISTS;
        goto fail;
    } else if (sret == SQLITE_ROW) {
        uio_debug("Found Cache entry with size (%lu) and name (%s)", fsize, fname);
    } else {
        uio_error("sqlite_step failed: %s", sqlite3_errmsg(cache_db));
        err = ERR_CACHE_SQLITE;
        goto fail;
    }

    if (!out_ce)
        goto end;
    if (sel == 0)
        sel = 0xFFFFFFFF;

    if (sel & CACHE_S_LID)
        out_ce->lid = sqlite3_column_int64(smt, 0);
    if (sel & CACHE_S_FNAME)
        out_ce->fname = strdup((const char *)sqlite3_column_text(smt, 1));
    if (sel & CACHE_S_FSIZE)
        out_ce->fsize = sqlite3_column_int64(smt, 2);
    if (sel & CACHE_S_ED2K) {
        const char *txt = (const char *)sqlite3_column_text(smt, 3);

        util_hex2byte(txt, out_ce->ed2k);
    }
    if (sel & CACHE_S_WATCHDATE)
        out_ce->wdate = sqlite3_column_int64(smt, 4);
    if (sel & CACHE_S_STATE)
        out_ce->state = sqlite3_column_int(smt, 5);
    if (sel & CACHE_S_MODDATE)
        out_ce->moddate = sqlite3_column_int64(smt, 6);

end:
fail:
    sqlite3_finalize(smt);
    return err;
}

bool cache_exists(const char *fname, uint64_t size)
{
    return cache_get(fname, size, 0, NULL) == NOERR;
}
