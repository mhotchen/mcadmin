#include <cdk/cdk.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "memcache/connect.h"
#include "memcache/commands.h"

void usage(const char *const filename);
void showStats(WINDOW *const win, const stats *const s);

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

    WINDOW *const statsWin = newwin(LINES, COLS / 2, 0, COLS / 2);
    WINDOW *const menuWin = newwin(LINES, COLS / 2, 0, 0);

    const struct timespec loopDelay = {0, 100000000};
    int command = 0;
    bool quit = false;

    while (!quit) {
        getStats(&s, sockfd);

        box(statsWin, 0, 0);
        showStats(statsWin, &s);
        wrefresh(statsWin);

        box(menuWin, 0, 0);
        mvwprintw(menuWin, 1, 1, "q: quit");
        mvwprintw(menuWin, 2, 1, "f: flush all content");
        mvwprintw(menuWin, 3, 1, "/: find");
        wrefresh(menuWin);

        command = getch();
        if (command == 'q') {
            quit = true;
        }
        if (command == 'f') {
            char *buttons[] = {"Yes", "No"};
            char *message[] = {"Are you sure you want to invalidate all entries in your memcache instance?"};
            CDKDIALOG *confirm = newCDKDialog(cdkScreen, CENTER, CENTER, message, 1, buttons, 2, A_REVERSE, true, true, true);
            int selection = activateCDKDialog(confirm, 0);
            if (confirm->exitType == vNORMAL) {
                switch (selection) {
                    case 0:
                        flushAll(sockfd);
                        break;
                    case 1:
                    default:
                        break;
                }
            }
        }
        if (command == '/') {
            char *message = "Type key and press enter. Leave search blank to cancel";
            char *label = "Key: ";
            CDKENTRY *search = newCDKEntry(cdkScreen, CENTER, CENTER, message, label, A_NORMAL, '.', vMIXED, 60, 0, 256, true, true);
            const char const *key = activateCDKEntry(search, 0);
            if (key != 0) {
                item i = {0};
                bool found = getItem(&i, key, sockfd);
                if (found) {
                    CDKSCROLL *view = newCDKScroll(cdkScreen, CENTER, CENTER, RIGHT, 80, 80, key, i.value, i.lines, true, A_REVERSE, true, true);
                    activateCDKScroll(view, 0);
                    char *buttons[] = {"Close", "Delete"};
                    char *message[] = {"Do you want to delete this item?"};
                    CDKDIALOG *confirm = newCDKDialog(cdkScreen, CENTER, CENTER, message, 1, buttons, 2, A_REVERSE, true, true, true);
                    int selection = activateCDKDialog(confirm, 0);
                    if (confirm->exitType == vNORMAL && selection == 1) {
                        // TODO delete
                    }
                } else {
                    char *buttons[] = {"Close"};
                    char *message[] = {"Item not found"};
                    CDKDIALOG *notFound = newCDKDialog(cdkScreen, CENTER, CENTER, message, 1, buttons, 2, A_REVERSE, true, true, true);
                    activateCDKDialog(notFound, 0);
                }
            }
        }

        refresh();

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
    mvwprintw(win, ++row, 1, "PID: %ld", s->pid);
    mvwprintw(win, ++row, 1, "Version: %s", s->version);
    mvwprintw(win, ++row, 1, "Uptime: %ld secs", s->uptime);
    mvwprintw(win, ++row, 1, "Usage: %.2f (usr), %.2f (sys)", s->rusage_user, s->rusage_system);
    mvwprintw(win, ++row, 1, "Connections: %ld (curr), %ld (tot)", s->curr_connections, s->total_connections);
    mvwprintw(win, ++row, 1, "Bytes: %ld (read), %ld (writ)", s->bytes_read, s->bytes_written);
    mvwprintw(win, ++row, 1, "       %ld (used), %ld (max)", s->bytes, s->limit_maxbytes);
    mvwprintw(win, ++row, 1, "Hit/miss:");
    mvwprintw(win, ++row, 1, "  get    %ldh/%ldm", s->get_hits, s->get_misses);
    mvwprintw(win, ++row, 1, "  delete %ldh/%ldm", s->delete_hits, s->delete_misses);
    mvwprintw(win, ++row, 1, "  cas    %ldh/%ldm/%ldb", s->cas_hits, s->cas_misses, s->cas_badval);
    mvwprintw(win, ++row, 1, "  incr   %ldh/%ldm", s->incr_hits, s->incr_misses);
    mvwprintw(win, ++row, 1, "  decr   %ldh/%ldm", s->decr_hits, s->decr_misses);
    mvwprintw(win, ++row, 1, "  touch  %ldh/%ldm", s->touch_hits, s->touch_misses);
}
