#include <cdk/cdk.h>
#include <ncurses.h>
#include <panel.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "actions/actions.h"
#include "memcache/connect.h"
#include "memcache/stats.h"
#include "ui/screens.h"

static void
usage(const char filename[static 1])
{
    fprintf(stderr, "Usage:\n\n");
    fprintf(stderr, "%s [HOST] [PORT]\n\n", filename);
    fprintf(stderr, "  [HOST] host address of the memcached host server either as IP or domain lookup\n");
    fprintf(stderr, "  [PORT] port number of the memcached instance to connect to\n\n");
}

static void
configureCdk(CDKSCREEN *screen)
{
    cbreak();
    noecho();
    nodelay(screen->window, true);
    curs_set(0);
}

static void
cleanup(CDKSCREEN *screen, int mcConn)
{
    destroyCDKScreen(screen);
    endCDK();
    close(mcConn);
}

int
main(const int argc, const char *const argv[argc])
{
    if (argc != 3) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    CDKSCREEN             *cdkScreen       = initCDKScreen(initscr());
    int                   mcConn           = 0;
    WINDOW                *menuWin         = newwin(1, COLS, 0, 0);
    long                  lastDataRefresh  = 0;
    PANEL                 *statsPanels[]   = {new_panel(newwin(LINES - 1, COLS, 1, 0))};
    PANEL                 *slabsPanels[]   = {new_panel(newwin(LINES - 1, COLS, 1, 0))};
    Screen                *statsScreen     = createScreen(1, statsPanels, NULL, &refreshStatsView);
    Screen                *slabsScreen     = createScreen(1, slabsPanels, statsScreen, &refreshSlabsView);
    struct timespec       loopDelay        = {0, 100000000};
    enum McConnectStatus  connectStatus    = connectByNetworkSocket(argv[1], argv[2], &mcConn);
    enum ActionStatus     actionStatus     = ACTION_STATUS_NO_ACTION;

    if (connectStatus != MC_CONN_STATUS_SUCCESS) {
        cleanup(cdkScreen, mcConn);
        switch (connectStatus) {
            case MC_CONN_STATUS_LOOKUP_ERROR:
                fprintf(stderr, "Unable to find host\n");
                break;
            case MC_CONN_STATUS_CANT_CONNECT:
                fprintf(stderr, "Host found but unable to connect (maybe wrong port?)\n");
                break;
            default:
                fprintf(stderr, "Unknown error connecting to memcache server. Error code %d\n", connectStatus);
                break;
        }
        exit(EXIT_FAILURE);
    }

    configureCdk(cdkScreen);

    statsScreen->next = slabsScreen;
    Screen *currentScreen = statsScreen;

    mvwprintw(menuWin, 0, 0, "mcadmin | q: quit | f: flush all content | /: find | s: switch view");
    wrefresh(menuWin);

    do {
        if (time(0) - lastDataRefresh > 3) {
            if (refreshStats(mcConn) != MC_COMMAND_STATUS_SUCCESS ||
                refreshSlabs(mcConn) != MC_COMMAND_STATUS_SUCCESS
            ) {
                cleanup(cdkScreen, mcConn);
                fprintf(stderr, "Error refreshing screen, likely because the connection to memcache was lost.\n");
                exit(EXIT_FAILURE);
            }

            lastDataRefresh = time(0);

        }

        currentScreen->refreshView(currentScreen);

        refreshCDKWindow(currentScreen->currentPanel->win);
        refreshCDKWindow(menuWin);

        nanosleep(&loopDelay, 0);

        actionStatus = handleAction(getch(), cdkScreen, mcConn, &currentScreen);
    } while (actionStatus == ACTION_STATUS_NO_ACTION || actionStatus == ACTION_STATUS_OK);

    cleanup(cdkScreen, mcConn);

    exit(actionStatus == ACTION_STATUS_QUIT ? EXIT_SUCCESS : EXIT_FAILURE);
}
