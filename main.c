#include <cdk/cdk.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "memcache/connect.h"
#include "memcache/commands.h"

void usage(const char *const filename);
void showStats(WINDOW *const win, const stats *const s);
void refreshWindows(WINDOW *const windows[], int count);

int
main(const int argc, const char *const argv[argc])
{
    if (argc != 3) {
        usage(argv[0]);
    }

    const int sockfd = connectByNetworkSocket(argv[1], argv[2]);
    stats s = {0};

    WINDOW* cursesWin = initscr();
    CDKSCREEN* cdkScreen = initCDKScreen(cursesWin);
    cbreak();
    noecho();
    nodelay(cursesWin, true);
    curs_set(0);
    getch();

    WINDOW *const menuWin = newwin(1, COLS, 0, 0);
    WINDOW *const statsWin = newwin(LINES - 1, COLS, 1, 0);

    WINDOW *const windows[] = {menuWin, statsWin};

    const struct timespec loopDelay = {0, 100000000};
    int command = 0;
    long lastStatsRefresh = 0;
    bool quit = false;

    mvwprintw(menuWin, 0, 0, "mcadmin | q: quit | f: flush all content | /: find");
    wrefresh(menuWin);

    while (!quit) {
        command = getch();
        if (command == 'q') {
            quit = true;
        }
        if (command == 'f') {
            char *buttons[] = {"Yes", "No"};
            char *message[] = {"Are you sure you want to invalidate all entries in your memcache instance?"};
            CDKDIALOG *confirm = newCDKDialog(cdkScreen, CENTER, CENTER, message, 1, buttons, 2, A_STANDOUT, true, true, false);
            int selection = activateCDKDialog(confirm, 0);
            if (confirm->exitType == vNORMAL && selection == 0) {
                flushAll(sockfd);
            }
        }
        if (command == '/') {
            char *message = "Type key and press enter. Leave search blank to cancel";
            char *label = "Key: ";
            CDKENTRY *search = newCDKEntry(cdkScreen, CENTER, CENTER, message, label, A_STANDOUT, '.', vMIXED, 60, 0, 256, true, false);
            char *key = activateCDKEntry(search, 0);
            if (key != 0 && strcmp(key, "") != 0) {
                item i = {0};
                bool found = getItem(&i, key, sockfd);
                if (found) {
                    char *buttons[] = {"View", "Delete", "Cancel"};
                    char *message[] = {"What action would you like to take with this item?"};
                    CDKDIALOG *confirm = newCDKDialog(cdkScreen, CENTER, CENTER, message, 1, buttons, 3, A_STANDOUT, true, true, false);
                    int selection = activateCDKDialog(confirm, 0);
                    if (confirm->exitType == vNORMAL) {
                        switch (selection) {
                            case 0: {
                                CDKSCROLL *view = newCDKScroll(cdkScreen, CENTER, CENTER, RIGHT, 80, 80, key, i.value, i.lines, true, A_STANDOUT, true, false);
                                activateCDKScroll(view, 0);
                                break;
                            }
                            case 1: {
                                char *buttons[] = {"Close"};
                                char *message[] = {deleteItem(key, sockfd) ? "Deleted"
                                                                           : "Couldn't delete. Maybe the item doesn't exist anymore"};
                                CDKDIALOG *notFound = newCDKDialog(cdkScreen, CENTER, CENTER, message, 1, buttons, 2,
                                                                   A_STANDOUT, true, true, false);
                                activateCDKDialog(notFound, 0);
                                break;
                            }
                            case 2:
                            default:
                                break;
                        }
                    }
                } else {
                    char *buttons[] = {"Close"};
                    char *message[] = {"Item not found"};
                    CDKDIALOG *notFound = newCDKDialog(cdkScreen, CENTER, CENTER, message, 1, buttons, 2, A_STANDOUT, true, true, false);
                    activateCDKDialog(notFound, 0);
                }
            }
        }

        if (command != -1) {
            refreshWindows(windows, sizeof(windows) / sizeof(windows[0]));
        }

        if (time(0) - lastStatsRefresh > 3) {
            getStats(&s, sockfd);
            showStats(statsWin, &s);
            wrefresh(statsWin);
            lastStatsRefresh = time(0);
        }

        nanosleep(&loopDelay, 0);
    }

    destroyCDKScreen(cdkScreen);
    endCDK();

    close(sockfd);
    exit(EXIT_SUCCESS);
}

void
usage(const char *const filename)
{
    fprintf(stderr, "Usage:\n\n");
    fprintf(stderr, "%s [HOST] [PORT]\n\n", filename);
    fprintf(stderr, "  [HOST] host address of the memcached host server either as IP or domain lookup\n");
    fprintf(stderr, "  [PORT] port number of the memcached instance to connect to\n\n");
    exit(EXIT_FAILURE);
}

void
showStats(WINDOW *const win, const stats *const s)
{
    int row = 0;
    mvwprintw(win,   row, 0, "Memcache %s, PID: %ld, uptime: %ld", s->version, s->pid, s->uptime);
    mvwprintw(win, ++row, 0, "CPU time: %.2f (usr), %.2f (sys)", s->rusage_user, s->rusage_system);
    mvwprintw(win, ++row, 0, "Memory: used: %ld bytes, available: %ld bytes", s->bytes, s->limit_maxbytes);
    mvwprintw(win, ++row, 0, "Network: read: %ld bytes, written: %ld bytes", s->bytes_read, s->bytes_written);
    mvwprintw(win, ++row, 0, "Connections: %ld (curr), %ld (tot)", s->curr_connections, s->total_connections);
    mvwprintw(win, ++row, 0, "Commands: set: %ld, get: %ldh/%ldm, delete %ldh/%ldm, cas %ldh/%ldm/%ldb", s->cmd_set, s->get_hits, s->get_misses, s->delete_hits, s->delete_misses, s->cas_hits, s->cas_misses, s->cas_badval);
    mvwprintw(win, ++row, 0, "          incr: %ldh/%ldm, decr %ldh/%ldm, touch %ldh/%ldm", s->incr_hits, s->incr_misses, s->decr_hits, s->decr_misses, s->touch_hits, s->touch_misses);
}

void
refreshWindows(WINDOW *const windows[], int count)
{
    for (int i = 0; i < count; ++i) {
        refreshCDKWindow(windows[i]);
    }
}
