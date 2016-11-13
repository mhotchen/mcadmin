#include "commands.h"

/**
 * Will invalidate all existing cache entries in the memcache server we're
 * connected to. Note that we may have lost a connection.
 */
enum McCommandStatus
flushAll(int mcConn)
{
    ssize_t recd            = 0;
    char    buff[BUFF_SIZE];

    send(mcConn, "flush_all\r\n", 11, 0);
    recd = recv(mcConn, buff, BUFF_SIZE, 0);

    if (recd == -1 || recd == 0) {
        return MC_COMMAND_STATUS_LOST_CONNECTION;
    }

    return MC_COMMAND_STATUS_SUCCESS;
}

/**
 * An item looks like: VALUE test 0 4\r\nTEST\r\nEND
 * where:
 * test = the key
 * 0 = flags
 * 4 = content length in bytes
 * TEST = actual data which may be multi-line and could contain the string
 *        \r\nEND\r\n at the end so the content length is important
 *
 * Because the data might be very large, we don't store the whole thing instead
 * going up to MAX_CONTENT_LENGTH in bytes. Instead of being simple and
 * allocating MAX_CONTENT_LENGTH on every request, I instead start at BUFF_SIZE
 * and increase allocation until MAX_CONTENT_LENGTH if necessary. This reduces
 * the memory footprint quite a lot in situations where most values are
 * smaller than MAX_CONTENT_LENGTH (which I estimate to be most situations)
 * but it does add the risk of memory errors occuring. We may also lose the
 * memcache connection.
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
                        status = MC_COMMAND_STATUS_MEMORY_ERROR;
                        goto end;
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
                        status = MC_COMMAND_STATUS_MEMORY_ERROR;
                        goto end;
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
                        status = MC_COMMAND_STATUS_MEMORY_ERROR;
                        goto end;
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
    if (segment) {
        free(segment);
    }

    if (contentSet) {
        memcpy(item->key, key, keyLength);
        item->key[keyLength] = '\0';
    }

    return status;
}

/**
 * Deletes an item from the memcache server. Note we may have lost the
 * connection, or the item might not exist.
 */
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
