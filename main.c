#include <cdk/cdk.h>
#include <ncurses.h>
#include <panel.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "memcache/commands.h"
#include "memcache/connect.h"

void usage(const char filename[static 1]);
void showStats(WINDOW *const win, const Stats *const stats);
void showSlab(WINDOW *const win, const Slab *const slab, const int i, const int slabCount);
void refreshWindows(int count, WINDOW *const windows[count]);

int
main(const int argc, const char *const argv[argc])
{
    if (argc != 3) {
        usage(argv[0]);
    }

    const int sockfd = connectByNetworkSocket(argv[1], argv[2]);
    Stats stats = {0};

    WINDOW *const cursesWin = initscr();
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

    mvwprintw(menuWin, 0, 0, "mcadmin | q: quit | f: flush all content | /: find | s: view slabs");
    wrefresh(menuWin);

    while (!quit) {
        command = getch();
        if (command == 'q') {
            quit = true;
        }
        if (command == 'f') {
            char *buttons[] = {"Yes", "No"};
            char *message[] = {"Are you sure you want to invalidate all entries in your memcache instance?"};
            CDKDIALOG *confirm = newCDKDialog(cdkScreen, CENTER, CENTER, message, 1, buttons, 2, A_REVERSE, true, true, false);
            int selection = activateCDKDialog(confirm, 0);
            if (confirm->exitType == vNORMAL && selection == 0) {
                flushAll(sockfd);
            }
        }
        if (command == '/') {
            char *message = "Type key and press enter. Leave search blank to cancel";
            char *label = "Key: ";
            CDKENTRY *search = newCDKEntry(cdkScreen, CENTER, CENTER, message, label, A_REVERSE, '.', vMIXED, 60, 0, 256, true, false);
            char *key = activateCDKEntry(search, 0);
            if (key != 0 && strcmp(key, "") != 0) {
                Item item = {0};
                bool found = getItem(&item, key, sockfd);
                if (found) {
                    char *buttons[] = {"View", "Delete", "Cancel"};
                    char *message[] = {"What action would you like to take with this item?"};
                    CDKDIALOG *confirm = newCDKDialog(cdkScreen, CENTER, CENTER, message, 1, buttons, 3, A_REVERSE, true, true, false);
                    int selection = activateCDKDialog(confirm, 0);
                    if (confirm->exitType == vNORMAL) {
                        switch (selection) {
                            case 0: {
                                char *title = calloc(sizeof(item.key) + 2 + 14, 0); /* 2 bytes for the 32bit flag, 14 for the extra characters below */
                                sprintf(title, "Key: %s, flags: %d", item.key, item.flags);
                                CDKSCROLL *view = newCDKScroll(cdkScreen, CENTER, CENTER, RIGHT, 80, 80, title, item.value, item.lines, true, A_REVERSE, true, false);
                                activateCDKScroll(view, 0);
                                free(title);
                                break;
                            }
                            case 1: {
                                char *buttons[] = {"Close"};
                                char *message[] = {deleteItem(key, sockfd) ? "Deleted" : "Couldn't delete. Maybe the item doesn't exist anymore"};
                                CDKDIALOG *notFound = newCDKDialog(cdkScreen, CENTER, CENTER, message, 1, buttons, 2, A_REVERSE, true, true, false);
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
                    CDKDIALOG *notFound = newCDKDialog(cdkScreen, CENTER, CENTER, message, 1, buttons, 2, A_REVERSE, true, true, false);
                    activateCDKDialog(notFound, 0);
                }
            }
        }
        if (command == 's') {
            Slab slab = {0};
            int slabCount = getSlabs(&slab, sockfd);
            WINDOW *slabWindows[slabCount];
            PANEL  *slabPanels[slabCount];
            PANEL  *currentTab;
            Slab   *currentSlab = &slab;

            for (int i = 0; i < slabCount; ++i) {
                slabWindows[i] = newwin(LINES - 1, COLS, 1, 0);
                slabPanels[i] = new_panel(slabWindows[i]);
                if (i != 0) {
                    set_panel_userptr(slabPanels[i - 1], slabPanels[i]);
                }
                showSlab(slabWindows[i], currentSlab, i + 1, slabCount);
                currentSlab = currentSlab->next;
            }

            set_panel_userptr(slabPanels[slabCount - 1], slabPanels[0]);
            currentTab = slabPanels[0];
            while(1) {
                command = getch();
                if (command == 'q') {
                    quit = true;
                    break;
                }
                if (command == 27 || command == '\r' || command == '\n') {
                    break;
                }
                if (command == 9) {
                    currentTab = (PANEL *) panel_userptr(currentTab);
                }
                top_panel(currentTab);
                update_panels();
                doupdate();
                nanosleep(&loopDelay, 0);
            }
        }

        if (command != -1) {
            refreshWindows(sizeof(windows) / sizeof(windows[0]), windows);
        }

        if (time(0) - lastStatsRefresh > 3) {
            getStats(&stats, sockfd);
            showStats(statsWin, &stats);
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
usage(const char filename[static 1])
{
    fprintf(stderr, "Usage:\n\n");
    fprintf(stderr, "%s [HOST] [PORT]\n\n", filename);
    fprintf(stderr, "  [HOST] host address of the memcached host server either as IP or domain lookup\n");
    fprintf(stderr, "  [PORT] port number of the memcached instance to connect to\n\n");
    exit(EXIT_FAILURE);
}

void
showStats(WINDOW *const win, const Stats *const stats)
{
    int row = 0;
    mvwprintw(win,   row, 0, "Memcache %s, PID: %ld, uptime: %ld", stats->version, stats->pid, stats->uptime);
    mvwprintw(win, ++row, 0, "CPU time: %.2f (usr), %.2f (sys)", stats->rusage_user, stats->rusage_system);
    mvwprintw(win, ++row, 0, "Memory: used: %ld bytes, available: %ld bytes", stats->bytes, stats->limit_maxbytes);
    mvwprintw(win, ++row, 0, "Network: read: %ld bytes, written: %ld bytes", stats->bytes_read, stats->bytes_written);
    mvwprintw(win, ++row, 0, "Connections: %ld (curr), %ld (tot)", stats->curr_connections, stats->total_connections);
    mvwprintw(win, ++row, 0, "Commands: set: %ld, get: %ldh/%ldm, delete %ldh/%ldm, cas %ldh/%ldm/%ldb", stats->cmd_set, stats->get_hits, stats->get_misses, stats->delete_hits, stats->delete_misses, stats->cas_hits, stats->cas_misses, stats->cas_badval);
    mvwprintw(win, ++row, 0, "          incr: %ldh/%ldm, decr %ldh/%ldm, touch %ldh/%ldm", stats->incr_hits, stats->incr_misses, stats->decr_hits, stats->decr_misses, stats->touch_hits, stats->touch_misses);
}

void
showSlab(WINDOW *const win, const Slab *const slab, const int i, const int slabTotal)
{
    int row = 0;
    mvwprintw(win,   row, 0, "Slab %ld (%d of %d)", slab->class, i, slabTotal);
    mvwprintw(win, ++row, 0, "Chunks: amount: %ld, used: %ld, free: %ld, chunk size: %ld", slab->total_chunks, slab->used_chunks, slab->free_chunks, slab->chunk_size);
    mvwprintw(win, ++row, 0, "Pages: amount: %ld, chunks per page: %ld", slab->total_pages, slab->chunks_per_page);
    mvwprintw(win, ++row, 0, "Memory: max: %ld, used: %ld, free: %ld", slab->total_chunks * slab->chunk_size, slab->mem_requested, (slab->total_chunks * slab->chunk_size) - slab->mem_requested);

}

void
refreshWindows(int count, WINDOW *const windows[count])
{
    for (int i = 0; i < count; ++i) {
        refreshCDKWindow(windows[i]);
    }
}
