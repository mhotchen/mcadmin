#ifndef MCADMIN_STATS_H
#define MCADMIN_STATS_H

#include <stdbool.h>

#define MAX_CONTENT_LENGTH 10000
#define BUFF_SIZE 80

typedef struct stats stats;
struct stats {
    long pid;
    long uptime;
    long time;
    char* version;
    char* libevent;
    long pointer_size;
    double rusage_user;
    double rusage_system;
    long curr_connections;
    long total_connections;
    long connection_structures;
    long reserved_fds;
    long cmd_get;
    long cmd_set;
    long cmd_flush;
    long cmd_touch;
    long get_hits;
    long get_misses;
    long delete_misses;
    long delete_hits;
    long incr_misses;
    long incr_hits;
    long decr_misses;
    long decr_hits;
    long cas_misses;
    long cas_hits;
    long cas_badval;
    long touch_hits;
    long touch_misses;
    long auth_cmds;
    long auth_errors;
    long bytes_read;
    long bytes_written;
    long limit_maxbytes;
    long accepting_conns;
    long listen_disabled_num;
    long time_in_listen_disabled_us;
    long threads;
    long conn_yields;
    long hash_power_level;
    long hash_bytes;
    long hash_is_expanding;
    long malloc_fails;
    long bytes;
    long curr_items;
    long total_items;
    long expired_unfetched;
    long evicted_unfetched;
    long evictions;
    long reclaimed;
    long crawler_reclaimed;
    long crawler_items_checked;
    long lrutail_reflocked;
};

typedef struct item item;
struct item {
    char* key;
    char* value[MAX_CONTENT_LENGTH];
    int length;
    int lines;
    int flags;
};

void getStats(stats *s, int sockfd);
void flushAll(int sockfd);
bool getItem(item *itemPtr, const char const key[static 1], int sockfd);
bool deleteItem(const char const key[static 1], int sockfd);

#endif //MCADMIN_STATS_H
