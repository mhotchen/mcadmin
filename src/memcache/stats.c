#include "stats.h"

/*
 * See https://github.com/memcached/memcached/blob/master/doc/protocol.txt
 * for detailed information on the memcached server's requests and responses.
 */

Stats stats     = {0};
Slab  slabs     = {0};
int   slabCount = 0;


static void setStat(const char const line[static 5]);
static long getSlabClass(const char const line[static 5]);
static void setSlabStat(Slab *slab, const char const line[static 5]);

/**
 * Retrieves the latest stats from memcache and places them in the external
 * `stats` variable defined in the header.
 *
 * A stats request to memcache will return something like this:
 *
 * STAT pid 1939
 * STAT uptime 232
 * STAT time 1478271822
 * STAT version 1.4.25 Ubuntu
 * STAT libevent 2.0.21-stable
 * ...
 *
 * So discard the first 5 characters from a line, capture up until the next
 * space as what stat to set, then the rest of the line is the value.
 *
 * I've gone for using a string buffer with increasing memory approach over
 * simply using fgets just because I'm not sure how long the longest line
 * might be. Because of the buffer approach it may be possible to hit memory
 * errors, but it's unlikely. The connection to memcache may also be lost.
 */
enum McStatStatus refreshStats(int mcConn)
{
    size_t            lineSize        = BUFF_SIZE;
    char              *line           = calloc(lineSize, sizeof(char));
    ssize_t           recd            = 0;
    int               i               = 0;
    enum McStatStatus status          = MC_STAT_STATUS_SUCCESS;
    char              buff[BUFF_SIZE];

    send(mcConn, "stats\r\n", 7, 0);

    while (1) {
        memset(buff, 0, BUFF_SIZE);
        recd = recv(mcConn, buff, BUFF_SIZE, 0);

        if (recd == -1 || recd == 0) {
            status = MC_STAT_STATUS_LOST_CONNECTION;
            goto end;
        }

        for (int j = 0; j < BUFF_SIZE; ++j) {
            if (buff[j] == '\r') {
                if (strcmp(line, "END") == 0) {
                    goto end;
                }

                setStat(line);
                memset(line, 0, lineSize);
                i = 0;
                ++j;
                continue;
            }

            if (buff[j] == '\n') {
                continue;
            }

            if (i == lineSize) {
                line = realloc(line, lineSize + BUFF_SIZE);
                memset(line + lineSize, 0, BUFF_SIZE);
                lineSize += BUFF_SIZE;
                if (!line) {
                    status = MC_STAT_STATUS_MEMORY_ERROR;
                    goto end;
                }
            }

            line[i] = buff[j];
            ++i;
        }
    }

end:
    if (line) {
        free(line);
    }
    return status;
}

/**
 * Retrieves the latest data for slabs and places it in the external `slabs`
 * variable defined in the header.
 *
 * A slabs request will return something like this:
 *
 * STAT 10:chunk_size 752
 * STAT 10:chunks_per_page 1394
 * STAT 10:total_pages 1
 * STAT 10:total_chunks 1394
 * STAT 10:used_chunks 0
 * STAT 10:free_chunks 1394
 * STAT 10:free_chunks_end 0
 * ...
 *
 * So discard the first five characters, capture up until the colon ':' as the
 * slab ID, then from the character after the colon to the space as the stat to
 * set, then the rest of the line as the value.
 *
 * Because there could be a dynamic number of slabs this is stored as a linked
 * list.
 *
 * I've gone for using a string buffer with increasing memory approach over
 * simply using fgets just because I'm not sure how long the longest line
 * might be. Because of the buffer approach it may be possible to hit memory
 * errors, but it's unlikely. The connection to memcache may also be lost.
 */
enum McStatStatus refreshSlabs(int mcConn)
{
    slabCount = 0;

    size_t            lineSize        = BUFF_SIZE;
    Slab              *current        = &slabs;
    char              buff[BUFF_SIZE] = {0};
    char              *line           = calloc(lineSize, sizeof(char));
    ssize_t           recd            = 0;
    int               i               = 0;
    enum McStatStatus status          = MC_STAT_STATUS_SUCCESS;

    if (!line) {
        status = MC_STAT_STATUS_MEMORY_ERROR;
        goto end;
    }

    send(mcConn, "stats slabs\r\n", 13, 0);

    while (1) {
        memset(buff, 0, BUFF_SIZE);
        recd = recv(mcConn, buff, BUFF_SIZE, 0);
        if (recd == -1 || recd == 0) {
            status = MC_STAT_STATUS_LOST_CONNECTION;
            goto end;
        }

        for (int j = 0; j < BUFF_SIZE; ++j) {
            if (buff[j] == '\r') {
                if (strcmp(line, "END") == 0) {
                    ++slabCount;
                    goto end;
                }

                long lineSlabClass = getSlabClass(line);

                if (current->class == 0 && lineSlabClass != -1) {
                    current->class = lineSlabClass;
                }

                if (lineSlabClass != -1 && lineSlabClass != current->class) {
                    current->next = malloc(sizeof(Slab));
                    if (!current->next) {
                        status = MC_STAT_STATUS_MEMORY_ERROR;
                        goto end;
                    }
                    memset(current->next, 0, sizeof(Slab));
                    current = current->next;
                    current->class = lineSlabClass;
                    ++slabCount;
                }

                setSlabStat(current, line);
                memset(line, 0, lineSize);
                i = 0;
                ++j;
                continue;
            }

            if (buff[j] == '\n') {
                continue;
            }

            if (i == lineSize) {
                line = realloc(line, lineSize + BUFF_SIZE);
                if (!line) {
                    status = MC_STAT_STATUS_MEMORY_ERROR;
                    goto end;
                }
                memset(line + lineSize, 0, BUFF_SIZE);
                lineSize += BUFF_SIZE;
            }

            line[i] = buff[j];
            ++i;
        }
    }

end:
    if (line) {
        free(line);
    }
    return status;
}

/**
 * A line looks like: STAT pid 4125
 *
 * So discard the first five characters, use up until the next space character
 * to determine what key to set, then the rest of the line is the value which
 * will be converted to the right type according to the memcache protocol.
 *
 * This value is then stored in the extern `stats` struct.
 */
static void
setStat(const char const line[static 5])
{
    size_t length        = strlen(line);
    int    sub           = 5;
    int    i             = sub;
    char   key[length];
    char   value[length];

    memset(key,   0, length);
    memset(value, 0, length);

    for (; i < length; ++i) {
        if (line[i] == ' ') {
            break;
        }

        key[i - sub] = line[i];
    }

    sub = ++i;

    for (; i < length; ++i) {
        value[i - sub] = line[i];
    }

    if (strcmp(key, "pid") == 0) {
        stats.pid = atol(value);
    } else if (strcmp(key, "uptime") == 0) {
        stats.uptime = atol(value);
    } else if (strcmp(key, "time") == 0) {
        stats.time = atol(value);
    } else if (strcmp(key, "version") == 0) {
        stats.version = malloc(sizeof(value));
        strcpy(stats.version, value);
    } else if (strcmp(key, "libevent") == 0) {
        stats.libevent = malloc(sizeof(value));
        strcpy(stats.version, value);
    } else if (strcmp(key, "pointer_size") == 0) {
        stats.pointer_size = atol(value);
    } else if (strcmp(key, "rusage_user") == 0) {
        stats.rusage_user = atof(value);
    } else if (strcmp(key, "rusage_system") == 0) {
        stats.rusage_system = atof(value);
    } else if (strcmp(key, "curr_connections") == 0) {
        stats.curr_connections = atol(value);
    } else if (strcmp(key, "total_connections") == 0) {
        stats.total_connections = atol(value);
    } else if (strcmp(key, "connection_structures") == 0) {
        stats.connection_structures = atol(value);
    } else if (strcmp(key, "reserved_fds") == 0) {
        stats.reserved_fds = atol(value);
    } else if (strcmp(key, "cmd_get") == 0) {
        stats.cmd_get = atol(value);
    } else if (strcmp(key, "cmd_set") == 0) {
        stats.cmd_set = atol(value);
    } else if (strcmp(key, "cmd_flush") == 0) {
        stats.cmd_flush = atol(value);
    } else if (strcmp(key, "cmd_touch") == 0) {
        stats.cmd_touch = atol(value);
    } else if (strcmp(key, "get_hits") == 0) {
        stats.get_hits = atol(value);
    } else if (strcmp(key, "get_misses") == 0) {
        stats.get_misses = atol(value);
    } else if (strcmp(key, "delete_misses") == 0) {
        stats.delete_misses = atol(value);
    } else if (strcmp(key, "delete_hits") == 0) {
        stats.delete_hits = atol(value);
    } else if (strcmp(key, "incr_misses") == 0) {
        stats.incr_misses = atol(value);
    } else if (strcmp(key, "incr_hits") == 0) {
        stats.incr_hits = atol(value);
    } else if (strcmp(key, "decr_misses") == 0) {
        stats.decr_misses = atol(value);
    } else if (strcmp(key, "decr_hits") == 0) {
        stats.decr_hits = atol(value);
    } else if (strcmp(key, "cas_misses") == 0) {
        stats.cas_misses = atol(value);
    } else if (strcmp(key, "cas_hits") == 0) {
        stats.cas_hits = atol(value);
    } else if (strcmp(key, "cas_badval") == 0) {
        stats.cas_badval = atol(value);
    } else if (strcmp(key, "touch_hits") == 0) {
        stats.touch_hits = atol(value);
    } else if (strcmp(key, "touch_misses") == 0) {
        stats.touch_misses = atol(value);
    } else if (strcmp(key, "auth_cmds") == 0) {
        stats.auth_cmds = atol(value);
    } else if (strcmp(key, "auth_errors") == 0) {
        stats.auth_errors = atol(value);
    } else if (strcmp(key, "bytes") == 0) {
        stats.bytes = atol(value);
    } else if (strcmp(key, "bytes_read") == 0) {
        stats.bytes_read = atol(value);
    } else if (strcmp(key, "bytes_written") == 0) {
        stats.bytes_written = atol(value);
    } else if (strcmp(key, "limit_maxbytes") == 0) {
        stats.limit_maxbytes = atol(value);
    } else if (strcmp(key, "accepting_conns") == 0) {
        stats.accepting_conns = atol(value);
    } else if (strcmp(key, "listen_disabled_num") == 0) {
        stats.listen_disabled_num = atol(value);
    } else if (strcmp(key, "time_in_listen_disabled_us") == 0) {
        stats.time_in_listen_disabled_us = atol(value);
    } else if (strcmp(key, "threads") == 0) {
        stats.threads = atol(value);
    } else if (strcmp(key, "conn_yields") == 0) {
        stats.conn_yields = atol(value);
    } else if (strcmp(key, "hash_power_level") == 0) {
        stats.hash_power_level = atol(value);
    } else if (strcmp(key, "hash_bytes") == 0) {
        stats.hash_bytes = atol(value);
    } else if (strcmp(key, "hash_is_expanding") == 0) {
        stats.hash_is_expanding = atol(value);
    }
}

/**
 * Given a line like the following: STAT 10:chunk_size 752
 *
 * The '10' before the colon ':' is the slab identifier/slab class. This
 * function will return that that identifier as a long, or return -1 if it
 * wasn't found (when getting slab stats there are some additional statistics
 * around slabs in general that aren't attached to a particular slab).
 */
static long
getSlabClass(const char const line[static 5])
{
    int    sub               = 5;
    size_t length            = strlen(line);
    bool   slabClassFound    = false;
    char   slabClass[length];

    memset(slabClass, 0, length);

    for (int i = sub; i < length; ++i) {
        if (line[i] == ':') {
            slabClassFound = true;
            break;
        }

        slabClass[i - sub] = line[i];
    }

    if (!slabClassFound) {
        return -1;
    }

    return atol(slabClass);
}

/**
 * A line looks like: STAT 7:chunk_size 384
 * or:                STAT active_slabs 4
 *
 * We disregard the second type of stat because it's not related to a slab
 *
 * The first five characters can be skipped, from there until the colon is the
 * slab class, which again can be skipped, then after the colon until the next
 * space is the key, then from there until the end of the line is the value
 * which is converted to the right type according to the memcache protocol.
 *
 * This value is then stored in the slab passed in.
 */
static void
setSlabStat(Slab *slab, const char const line[static 5])
{
    int    sub            = 5;
    int    i              = sub;
    size_t length         = strlen(line);
    bool   slabClassFound = false;
    char   key[length];
    char   value[length];

    memset(key,   0, length);
    memset(value, 0, length);

    for (; i < length; ++i) {
        if (line[i] == ':') {
            slabClassFound = true;
            break;
        }
    }

    sub = ++i;

    if (!slabClassFound) {
        return;
    }

    for (; i < length; ++i) {
        if (line[i] == ' ') {
            break;
        }

        key[i - sub] = line[i];
    }

    sub = ++i;

    for (; i < length; ++i) {
        value[i - sub] = line[i];
    }

    if (strcmp(key, "chunk_size") == 0) {
        slab->chunk_size = atol(value);
    } else if (strcmp(key, "chunks_per_page") == 0) {
        slab->chunks_per_page = atol(value);
    } else if (strcmp(key, "total_chunks") == 0) {
        slab->total_chunks = atol(value);
    } else if (strcmp(key, "total_pages") == 0) {
        slab->total_pages = atol(value);
    } else if (strcmp(key, "used_chunks") == 0) {
        slab->used_chunks = atol(value);
    } else if (strcmp(key, "free_chunks") == 0) {
        slab->free_chunks = atol(value);
    } else if (strcmp(key, "mem_requested") == 0) {
        slab->mem_requested = atol(value);
    }
}
