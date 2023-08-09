#include <stdio.h>

#include "error.h"
#include "api.h"
#include "util.h"
#include "uio.h"

enum error cmd_stats(void *data)
{
    enum error err = NOERR;
    struct api_result res;
    struct api_myliststats_result *stats = &res.myliststats;

    err = api_cmd_myliststats(&res);
    if (err != NOERR) {
        uio_error("Failed to request stats from API server");
        return err;
    }

    printf("Mylist status:\n"
            "Anime:    %lu\n"
            "Episodes: %lu\n"
            "Files:    %lu\n"
            "Size of files: %lu\n"
            "Added groups: %lu\n"
            "leech%%:   %lu\n"
            "Glory%%:   %lu\n"
            "Viewed%%/DB: %lu\n"
            "Mylist%%/DB: %lu\n"
            "Viewed%%/Mylist: %lu\n"
            "Viewed eps: %lu\n"
            "Votes: %lu\n"
            "Reviews: %lu\n"
            "Viewed minutes: %lu\n",
            stats->animes, stats->eps, stats->files, stats->size_of_files, stats->added_groups,
            stats->leech_prcnt, stats->glory_prcnt, stats->viewed_prcnt_of_db,
            stats->mylist_prcnt_of_db, stats->viewed_prcnt_of_mylist, stats->num_of_viewed_eps,
            stats->votes, stats->reviews, stats->viewed_minutes);
    return NOERR;
}
