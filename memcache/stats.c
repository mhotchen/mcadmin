#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "stats.h"

void
getStats(stats *s, int sockfd)
{
    size_t lineSize = BUFF_SIZE;

    char buff[BUFF_SIZE];
    char *line = calloc(lineSize, sizeof(char));
    int recd = 0;
    int i = 0;

    send(sockfd, "stats\n", 6, 0);
    while (1) {
        memset(buff, 0, BUFF_SIZE);
        recd = (int) recv(sockfd, buff, BUFF_SIZE, 0);
        if (recd == 0) {
            break;
        }

        if (recd == -1) {
            perror("Lost connection to memcache server\n");
            exit(EXIT_FAILURE);
        }

        for (int j = 0; j < BUFF_SIZE; ++j) {
            if (buff[j] == '\r' && j + 1 < BUFF_SIZE && buff[j + 1] == '\n') {
                if (strcmp(line, "END") == 0) {
                    return;
                }

                setStat(s, line);
                memset(line, 0, lineSize);
                i = 0;
                ++j;
                continue;
            }

            if (i == lineSize) {
                lineSize;
                line = realloc(line, lineSize + BUFF_SIZE);
                memset(line + lineSize, 0, BUFF_SIZE);
                lineSize += BUFF_SIZE;
                if (!line) {
                    perror("Couldn't allocate memory when retrieving stats\n");
                    exit(EXIT_FAILURE);
                }
            }

            line[i] = buff[j];
            ++i;
        }
    }
}

/*
 * A line looks like: STAT pid 4125
 */
void
setStat(stats *s, const char const line[static 5])
{
    size_t length = strlen(line);
    int sub = 5;
    int i = sub;

    char key[length];
    char value[length];
    memset(key, 0, length);
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

    /* I'm not super happy about this duplication but my C-fu isn't good enough to find a better method */

    if (strcmp(key, "pid") == 0) {
        s->pid = atol(value);
    }
    else if(strcmp(key, "uptime") == 0) {
        s->uptime = atol(value);
    }
    else if(strcmp(key, "time") == 0) {
        s->time = atol(value);
    }
    else if(strcmp(key, "version") == 0) {
        s->version = (char *) malloc(sizeof(value));
        strcpy(s->version, value);
    }
    else if(strcmp(key, "libevent") == 0) {
        s->libevent = (char *) malloc(sizeof(value));
        strcpy(s->version, value);
    }
    else if(strcmp(key, "pointer_size") == 0) {
        s->pointer_size = atol(value);
    }
    else if(strcmp(key, "rusage_user") == 0) {
        s->rusage_user = atof(value);
    }
    else if(strcmp(key, "rusage_system") == 0) {
        s->rusage_system = atof(value);
    }
    else if(strcmp(key, "curr_connections") == 0) {
        s->curr_connections = atol(value);
    }
    else if(strcmp(key, "total_connections") == 0) {
        s->total_connections = atol(value);
    }
    else if(strcmp(key, "connection_structures") == 0) {
        s->connection_structures = atol(value);
    }
    else if(strcmp(key, "reserved_fds") == 0) {
        s->reserved_fds = atol(value);
    }
    else if(strcmp(key, "cmd_get") == 0) {
        s->cmd_get = atol(value);
    }
    else if(strcmp(key, "cmd_set") == 0) {
        s->cmd_set = atol(value);
    }
    else if(strcmp(key, "cmd_flush") == 0) {
        s->cmd_flush = atol(value);
    }
    else if(strcmp(key, "cmd_touch") == 0) {
        s->cmd_touch = atol(value);
    }
    else if(strcmp(key, "get_hits") == 0) {
        s->get_hits = atol(value);
    }
    else if(strcmp(key, "get_misses") == 0) {
        s->get_misses = atol(value);
    }
    else if(strcmp(key, "delete_misses") == 0) {
        s->delete_misses = atol(value);
    }
    else if(strcmp(key, "delete_hits") == 0) {
        s->delete_hits = atol(value);
    }
    else if(strcmp(key, "incr_misses") == 0) {
        s->incr_misses = atol(value);
    }
    else if(strcmp(key, "incr_hits") == 0) {
        s->incr_hits = atol(value);
    }
    else if(strcmp(key, "decr_misses") == 0) {
        s->decr_misses = atol(value);
    }
    else if(strcmp(key, "decr_hits") == 0) {
        s->decr_hits = atol(value);
    }
    else if(strcmp(key, "cas_misses") == 0) {
        s->cas_misses = atol(value);
    }
    else if(strcmp(key, "cas_hits") == 0) {
        s->cas_hits = atol(value);
    }
    else if(strcmp(key, "cas_badval") == 0) {
        s->cas_badval = atol(value);
    }
    else if(strcmp(key, "touch_hits") == 0) {
        s->touch_hits = atol(value);
    }
    else if(strcmp(key, "touch_misses") == 0) {
        s->touch_misses = atol(value);
    }
    else if(strcmp(key, "auth_cmds") == 0) {
        s->auth_cmds = atol(value);
    }
    else if(strcmp(key, "auth_errors") == 0) {
        s->auth_errors = atol(value);
    }
    else if(strcmp(key, "bytes_read") == 0) {
        s->bytes_read = atol(value);
    }
    else if(strcmp(key, "bytes_written") == 0) {
        s->bytes_written = atol(value);
    }
    else if(strcmp(key, "limit_maxbytes") == 0) {
        s->limit_maxbytes = atol(value);
    }
    else if(strcmp(key, "accepting_conns") == 0) {
        s->accepting_conns = atol(value);
    }
    else if(strcmp(key, "listen_disabled_num") == 0) {
        s->listen_disabled_num = atol(value);
    }
    else if(strcmp(key, "time_in_listen_disabled_us") == 0) {
        s->time_in_listen_disabled_us = atol(value);
    }
    else if(strcmp(key, "threads") == 0) {
        s->threads = atol(value);
    }
    else if(strcmp(key, "conn_yields") == 0) {
        s->conn_yields = atol(value);
    }
    else if(strcmp(key, "hash_power_level") == 0) {
        s->hash_power_level = atol(value);
    }
    else if(strcmp(key, "hash_bytes") == 0) {
        s->hash_bytes = atol(value);
    }
    else if(strcmp(key, "hash_is_expanding") == 0) {
        s->hash_is_expanding = atol(value);
    }
}
