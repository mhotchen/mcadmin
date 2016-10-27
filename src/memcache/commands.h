#ifndef MCADMIN_COMMANDS_H
#define MCADMIN_COMMANDS_H

#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_CONTENT_LENGTH 102400
#define BUFF_SIZE 80

typedef struct Item Item;
struct Item {
    char key[251]; /* mc limit is 250, extra character for the null terminator */
    char *value[MAX_CONTENT_LENGTH]; /* TODO find a less memory intensive structure for this; maybe linked list? */
    int  length;
    int  lines;
    int  flags;
};

enum McCommandStatus {
    MC_COMMAND_STATUS_SUCCESS,
    MC_COMMAND_STATUS_LOST_CONNECTION,
    MC_COMMAND_STATUS_MEMORY_ERROR,
    MC_COMMAND_STATUS_ITEM_NOT_FOUND
};

enum McCommandStatus flushAll(int mcConn);
enum McCommandStatus getItem(Item *item, const char const key[static 1], int mcConn);
enum McCommandStatus deleteItem(const char const key[static 1], int mcConn);

#endif //MCADMIN_STATS_H
