#include <cdk/cdk.h>
#include <ncurses.h>
#include <panel.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "actions/actions.h"
#include "memcache/connect.h"
#include "ui/screens.h"

static void
usage(const char filename[static 1])
{
    fprintf(stderr, "Usage:\n\n");
    fprintf(stderr, "%s [HOST] [PORT]\n\n", filename);
    fprintf(stderr, "  [HOST] host address of the memcached host server either as IP or domain lookup\n");
    fprintf(stderr, "  [PORT] port number of the memcached instance to connect to\n\n");
    exit(EXIT_FAILURE);
}

int
main(const int argc, const char *const argv[argc])
{
    if (argc != 3) {
        usage(argv[0]);
    }

    int       mcConn = connectByNetworkSocket(argv[1], argv[2]);
    CDKSCREEN *cdkScreen = initCDKScreen(initscr());
    WINDOW    *menuWin = newwin(1, COLS, 0, 0);
    long      lastDataRefresh = 0;
    PANEL     *statsPanels[] = {new_panel(newwin(LINES - 1, COLS, 1, 0))};
    PANEL     *slabsPanels[] = {new_panel(newwin(LINES - 1, COLS, 1, 0))};
    Screen    *statsScreen = createScreen(1, statsPanels, NULL, &refreshStatsData);
    Screen    *slabsScreen = createScreen(1, slabsPanels, statsScreen, &refreshSlabsData);
    struct timespec loopDelay = {0, 100000000};

    statsScreen->next = slabsScreen;
    Screen *currentScreen = statsScreen;
    cbreak();
    noecho();
    nodelay(cdkScreen->window, true);
    curs_set(0);

    mvwprintw(menuWin, 0, 0, "mcadmin | q: quit | f: flush all content | /: find | s: switch view");
    wrefresh(menuWin);

    while (currentScreen) {
        if (time(0) - lastDataRefresh > 3) {
            currentScreen->refreshData(currentScreen, mcConn);
            lastDataRefresh = time(0);
        }

        refreshCDKWindow(currentScreen->currentPanel->win);
        refreshCDKWindow(menuWin);

        nanosleep(&loopDelay, 0);

        handleAction(getch(), cdkScreen, mcConn, &currentScreen);
    }

    destroyCDKScreen(cdkScreen);
    endCDK();

    close(mcConn);
    exit(EXIT_SUCCESS);
}
