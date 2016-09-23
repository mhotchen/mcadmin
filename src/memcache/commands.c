#include "commands.h"

static void setStat(Stats *stats, const char const line[static 5]);
static long getSlabClass(const char const line[static 5]);
static void setSlabStat(Slab *slab, const char const line[static 5]);

enum McCommandStatus
getStats(Stats *stats, int mcConn)
{
    size_t               lineSize        = BUFF_SIZE;
    char                 *line           = calloc(lineSize, sizeof(char));
    ssize_t              recd            = 0;
    int                  i               = 0;
    enum McCommandStatus status          = MC_COMMAND_STATUS_SUCCESS;
    char                 buff[BUFF_SIZE];

    send(mcConn, "stats\r\n", 7, 0);

    while (1) {
        memset(buff, 0, BUFF_SIZE);
        recd = recv(mcConn, buff, BUFF_SIZE, 0);

        if (recd == -1 || recd == 0) {
            status = MC_COMMAND_STATUS_LOST_CONNECTION;
            goto end;
        }

        for (int j = 0; j < BUFF_SIZE; ++j) {
            if (buff[j] == '\r') {
                if (strcmp(line, "END") == 0) {
                    goto end;
                }

                setStat(stats, line);
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
                    return MC_COMMAND_STATUS_MEMORY_ERROR;
                }
            }

            line[i] = buff[j];
            ++i;
        }
    }

end:
    free(line);
    return status;
}

enum McCommandStatus
flushAll(int mcConn)
{
    ssize_t recd            = 0;
    char    buff[BUFF_SIZE];

    send(mcConn, "flush_all noreply\r\n", 19, 0);
    recd = recv(mcConn, buff, BUFF_SIZE, 0);

    if (recd == -1 || recd == 0) {
        return MC_COMMAND_STATUS_LOST_CONNECTION;
    }

    return MC_COMMAND_STATUS_SUCCESS;
}

/*
 * An item looks like: VALUE test 0 4\r\nTEST\r\nEND
 * where:
 * test = the key
 * 0 = flags
 * 4 = content length in bytes
 * TEST = actual data which may be multi-line and could contain the string
 *        \r\nEND\r\n at the end so the content length is important
 *
 */
enum McCommandStatus
getItem(Item *item, const char const key[static 1], int mcConn)
{
    size_t               keyLength       = strlen(key);
    size_t               commandLength   = keyLength + 6; /* strlen("get \r\n") == 6 */
    ssize_t              recd            = 0;
    char                 *segment        = calloc(BUFF_SIZE, sizeof(char));
    char                 *command        = calloc(commandLength, sizeof(char));
    size_t               j               = keyLength + 7; /* strlen("VALUE  ") == 7 */
    size_t               i               = 0;
    int                  contentPos      = 0;
    int                  line            = 0;
    size_t               segmentSize     = BUFF_SIZE;
    enum McCommandStatus status          = MC_COMMAND_STATUS_ITEM_NOT_FOUND;
    bool                 flagsSet        = false;
    bool                 lengthSet       = false;
    bool                 contentSet      = false;
    char                 buff[BUFF_SIZE];

    if (!segment || ! command) {
        return MC_COMMAND_STATUS_MEMORY_ERROR;
    }

    sprintf(command, "get %s\r\n", key);
    send(mcConn, command, commandLength, 0);
    free(command);

    while (1) {
        memset(buff, 0, BUFF_SIZE);
        recd = recv(mcConn, buff, BUFF_SIZE, 0);

        if (recd == -1 || recd == 0) {
            status = MC_COMMAND_STATUS_LOST_CONNECTION;
            goto end;
        }

        if (strcmp("END\r\n", buff) == 0) {
            goto end;
        }

        status = MC_COMMAND_STATUS_SUCCESS;

        if (j >= BUFF_SIZE) {
            j -= BUFF_SIZE;
            continue;
        }

        if (!flagsSet) {
            for (; j < BUFF_SIZE; ++j) {
                if (buff[j] == ' ') {
                    item->flags = atoi(segment);
                    memset(segment, 0, segmentSize);
                    flagsSet = true;
                    ++j; /* skip the <space> */
                    i = 0;
                    break;
                }

                segment[i++] = buff[j];
                if (i == segmentSize) {
                    segment = realloc(segment, segmentSize + BUFF_SIZE);
                    if (!segment) {
                        return MC_COMMAND_STATUS_MEMORY_ERROR;
                    }
                    memset(segment + segmentSize, 0, BUFF_SIZE);
                    segmentSize += BUFF_SIZE;
                }
            }
        }

        if (flagsSet && !lengthSet) {
            for (; j < BUFF_SIZE; ++j) {
                if (buff[j] == '\r') {
                    item->length = atoi(segment);
                    memset(segment, 0, segmentSize);
                    lengthSet = true;
                    j += 2; /* skip the \r\n */
                    i = 0;
                    break;
                }

                segment[i++] = buff[j];
                if (i == segmentSize) {
                    segment = realloc(segment, segmentSize + BUFF_SIZE);
                    if (!segment) {
                        return MC_COMMAND_STATUS_MEMORY_ERROR;
                    }
                    memset(segment + segmentSize, 0, BUFF_SIZE);
                    segmentSize += BUFF_SIZE;
                }
            }
        }

        if (lengthSet && !contentSet) {
            for (; j < BUFF_SIZE; ++j) {
                if (contentPos == MAX_CONTENT_LENGTH || contentPos == item->length) {
                    item->value[line] = malloc(i);
                    item->lines = line + 1;
                    memcpy(item->value[line], segment, i);
                    contentSet = true;
                    break;
                }

                if (buff[j] == '\n') {
                    segment[i++] = 0;
                    item->value[line] = malloc(i);
                    memcpy(item->value[line], segment, i);
                    memset(segment, 0, segmentSize);
                    i = 0;
                    contentPos++;
                    line++;
                    continue;
                }

                segment[i++] = buff[j];
                if (i == segmentSize) {
                    segment = realloc(segment, segmentSize + BUFF_SIZE);
                    if (!segment) {
                        return MC_COMMAND_STATUS_MEMORY_ERROR;
                    }
                    memset(segment + segmentSize, 0, BUFF_SIZE);
                    segmentSize += BUFF_SIZE;
                }

                contentPos++;
            }
        }

        if (contentSet) {
            goto end;
        }

        j = 0;
    }

end:
    free(segment);

    memcpy(item->key, key, keyLength);
    item->key[keyLength] = '\0';

    return status;
}

enum McCommandStatus
deleteItem(const char const key[static 1], int mcConn)
{
    size_t  commandLength = strlen(key) + 9;
    ssize_t recd          = 0;
    char    *command      = calloc(commandLength, 0);
    char    buff[11]      = {0};

    sprintf(command, "delete %s\r\n", key);
    send(mcConn, command, commandLength, 0);
    free(command);

    recd = recv(mcConn, buff, 11, 0);

    if (recd == -1 || recd == 0) {
        return MC_COMMAND_STATUS_LOST_CONNECTION;
    }

    if (strcmp("DELETED\r\n", buff) == 0) {
        return MC_COMMAND_STATUS_SUCCESS;
    }

    return MC_COMMAND_STATUS_ITEM_NOT_FOUND;
}

enum McCommandStatus
getSlabs(Slab *first, int *count, int mcConn)
{
    size_t               lineSize        = BUFF_SIZE;
    Slab                 *current        = first;
    char                 buff[BUFF_SIZE] = {0};
    char                 *line           = calloc(lineSize, sizeof(char));
    ssize_t              recd            = 0;
    int                  i               = 0;
    enum McCommandStatus status          = MC_COMMAND_STATUS_SUCCESS;

    if (!line) {
        return MC_COMMAND_STATUS_MEMORY_ERROR;
    }

    send(mcConn, "stats slabs\r\n", 13, 0);

    while (1) {
        memset(buff, 0, BUFF_SIZE);
        recd = recv(mcConn, buff, BUFF_SIZE, 0);
        if (recd == -1 || recd == 0) {
            status = MC_COMMAND_STATUS_LOST_CONNECTION;
            goto end;
        }

        for (int j = 0; j < BUFF_SIZE; ++j) {
            if (buff[j] == '\r') {
                if (strcmp(line, "END") == 0) {
                    ++*count;
                    goto end;
                }

                long lineSlabClass = getSlabClass(line);

                if (current->class == 0 && lineSlabClass != -1) {
                    current->class = lineSlabClass;
                }

                if (lineSlabClass != -1 && lineSlabClass != current->class) {
                    current->next = malloc(sizeof(Slab));
                    if (!current->next) {
                        return MC_COMMAND_STATUS_MEMORY_ERROR;
                    }
                    memset(current->next, 0, sizeof(Slab));
                    current = current->next;
                    current->class = lineSlabClass;
                    ++*count;
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
                    return MC_COMMAND_STATUS_MEMORY_ERROR;
                }
                memset(line + lineSize, 0, BUFF_SIZE);
                lineSize += BUFF_SIZE;
            }

            line[i] = buff[j];
            ++i;
        }
    }

end:
    free(line);
    return status;
}

/*
 * A line looks like: STAT pid 4125
 */
static void
setStat(Stats *stats, const char const line[static 5])
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

    /*
     * I'm not super happy about this duplication but my C-fu isn't good
     * enough to find a better method
     */

    if (strcmp(key, "pid") == 0) {
        stats->pid = atol(value);
    } else if (strcmp(key, "uptime") == 0) {
        stats->uptime = atol(value);
    } else if (strcmp(key, "time") == 0) {
        stats->time = atol(value);
    } else if (strcmp(key, "version") == 0) {
        stats->version = malloc(sizeof(value));
        strcpy(stats->version, value);
    } else if (strcmp(key, "libevent") == 0) {
        stats->libevent = malloc(sizeof(value));
        strcpy(stats->version, value);
    } else if (strcmp(key, "pointer_size") == 0) {
        stats->pointer_size = atol(value);
    } else if (strcmp(key, "rusage_user") == 0) {
        stats->rusage_user = atof(value);
    } else if (strcmp(key, "rusage_system") == 0) {
        stats->rusage_system = atof(value);
    } else if (strcmp(key, "curr_connections") == 0) {
        stats->curr_connections = atol(value);
    } else if (strcmp(key, "total_connections") == 0) {
        stats->total_connections = atol(value);
    } else if (strcmp(key, "connection_structures") == 0) {
        stats->connection_structures = atol(value);
    } else if (strcmp(key, "reserved_fds") == 0) {
        stats->reserved_fds = atol(value);
    } else if (strcmp(key, "cmd_get") == 0) {
        stats->cmd_get = atol(value);
    } else if (strcmp(key, "cmd_set") == 0) {
        stats->cmd_set = atol(value);
    } else if (strcmp(key, "cmd_flush") == 0) {
        stats->cmd_flush = atol(value);
    } else if (strcmp(key, "cmd_touch") == 0) {
        stats->cmd_touch = atol(value);
    } else if (strcmp(key, "get_hits") == 0) {
        stats->get_hits = atol(value);
    } else if (strcmp(key, "get_misses") == 0) {
        stats->get_misses = atol(value);
    } else if (strcmp(key, "delete_misses") == 0) {
        stats->delete_misses = atol(value);
    } else if (strcmp(key, "delete_hits") == 0) {
        stats->delete_hits = atol(value);
    } else if (strcmp(key, "incr_misses") == 0) {
        stats->incr_misses = atol(value);
    } else if (strcmp(key, "incr_hits") == 0) {
        stats->incr_hits = atol(value);
    } else if (strcmp(key, "decr_misses") == 0) {
        stats->decr_misses = atol(value);
    } else if (strcmp(key, "decr_hits") == 0) {
        stats->decr_hits = atol(value);
    } else if (strcmp(key, "cas_misses") == 0) {
        stats->cas_misses = atol(value);
    } else if (strcmp(key, "cas_hits") == 0) {
        stats->cas_hits = atol(value);
    } else if (strcmp(key, "cas_badval") == 0) {
        stats->cas_badval = atol(value);
    } else if (strcmp(key, "touch_hits") == 0) {
        stats->touch_hits = atol(value);
    } else if (strcmp(key, "touch_misses") == 0) {
        stats->touch_misses = atol(value);
    } else if (strcmp(key, "auth_cmds") == 0) {
        stats->auth_cmds = atol(value);
    } else if (strcmp(key, "auth_errors") == 0) {
        stats->auth_errors = atol(value);
    } else if (strcmp(key, "bytes") == 0) {
        stats->bytes = atol(value);
    } else if (strcmp(key, "bytes_read") == 0) {
        stats->bytes_read = atol(value);
    } else if (strcmp(key, "bytes_written") == 0) {
        stats->bytes_written = atol(value);
    } else if (strcmp(key, "limit_maxbytes") == 0) {
        stats->limit_maxbytes = atol(value);
    } else if (strcmp(key, "accepting_conns") == 0) {
        stats->accepting_conns = atol(value);
    } else if (strcmp(key, "listen_disabled_num") == 0) {
        stats->listen_disabled_num = atol(value);
    } else if (strcmp(key, "time_in_listen_disabled_us") == 0) {
        stats->time_in_listen_disabled_us = atol(value);
    } else if (strcmp(key, "threads") == 0) {
        stats->threads = atol(value);
    } else if (strcmp(key, "conn_yields") == 0) {
        stats->conn_yields = atol(value);
    } else if (strcmp(key, "hash_power_level") == 0) {
        stats->hash_power_level = atol(value);
    } else if (strcmp(key, "hash_bytes") == 0) {
        stats->hash_bytes = atol(value);
    } else if (strcmp(key, "hash_is_expanding") == 0) {
        stats->hash_is_expanding = atol(value);
    }
}

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

/*
 * A line looks like: STAT 7:chunk_size 384
 * or:                STAT active_slabs 4
 *
 * We disregard the second type of stat because it's not related to a slab
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
