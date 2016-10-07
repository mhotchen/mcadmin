#ifndef MCADMIN_SCREENS_H
#define MCADMIN_SCREENS_H

#include <curses.h>
#include <malloc.h>
#include <panel.h>
#include "../memcache/commands.h"

enum RefreshStatus {
    REFRESH_STATUS_OK,
    REFRESH_STATUS_ERROR
};

typedef struct Screen Screen;
struct Screen {
    int                panelCount;
    PANEL              **panels;
    PANEL              *currentPanel;
    enum RefreshStatus (*refreshData)(Screen *this, int mcConn);
    Screen             *next;
};

Screen *createScreen(
    int panelCount,
    PANEL *panels[panelCount],
    Screen *next,
    enum RefreshStatus (*refreshData)(Screen *this, int mcConn)
);

enum RefreshStatus refreshStatsData(Screen *screen, int mcConn);
enum RefreshStatus refreshSlabsData(Screen *screen, int mcConn);

#endif //MCADMIN_SCREENS_H
