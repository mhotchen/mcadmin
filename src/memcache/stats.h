#ifndef MCADMIN_STATS_H
#define MCADMIN_STATS_H

#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BUFF_SIZE 80

typedef struct Stats Stats;
struct Stats {
    long   pid;
    long   uptime;
    long   time;
    char   *version;
    char   *libevent;
    long   pointer_size;
    double rusage_user;
    double rusage_system;
    long   curr_connections;
    long   total_connections;
    long   connection_structures;
    long   reserved_fds;
    long   cmd_get;
    long   cmd_set;
    long   cmd_flush;
    long   cmd_touch;
    long   get_hits;
    long   get_misses;
    long   delete_misses;
    long   delete_hits;
    long   incr_misses;
    long   incr_hits;
    long   decr_misses;
    long   decr_hits;
    long   cas_misses;
    long   cas_hits;
    long   cas_badval;
    long   touch_hits;
    long   touch_misses;
    long   auth_cmds;
    long   auth_errors;
    long   bytes_read;
    long   bytes_written;
    long   limit_maxbytes;
    long   accepting_conns;
    long   listen_disabled_num;
    long   time_in_listen_disabled_us;
    long   threads;
    long   conn_yields;
    long   hash_power_level;
    long   hash_bytes;
    long   hash_is_expanding;
    long   malloc_fails;
    long   bytes;
    long   curr_items;
    long   total_items;
    long   expired_unfetched;
    long   evicted_unfetched;
    long   evictions;
    long   reclaimed;
    long   crawler_reclaimed;
    long   crawler_items_checked;
    long   lrutail_reflocked;
};

typedef struct Slab Slab;
struct Slab {
    long  class;
    long  chunk_size;
    long  chunks_per_page;
    long  total_chunks;
    long  total_pages;
    long  used_chunks;
    long  free_chunks;
    long  mem_requested;
    Slab* next;
};

enum McStatStatus {
    MC_STAT_STATUS_SUCCESS,
    MC_STAT_STATUS_LOST_CONNECTION,
    MC_STAT_STATUS_MEMORY_ERROR,
};

extern Stats stats;
extern Slab  slabs;
extern int   slabCount;


enum McStatStatus refreshStats(int mcConn);
enum McStatStatus refreshSlabs(int mcConn);

#endif //MCADMIN_STATS_H
