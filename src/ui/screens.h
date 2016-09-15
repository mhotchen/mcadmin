#ifndef MCADMIN_SCREENS_H
#define MCADMIN_SCREENS_H

#include <curses.h>
#include <malloc.h>
#include <panel.h>
#include "../memcache/commands.h"

typedef struct Screen Screen;
struct Screen {
    int    panelCount;
    PANEL  **panels;
    PANEL  *currentPanel;
    void   (*refreshData)(Screen *this, int mcConn);
    Screen *next;
};

Screen *createScreen(int panelCount, PANEL *panels[panelCount], Screen *next, void (*refreshData)(Screen *this, int mcConn));
void   refreshStatsData(Screen *screen, int mcConn);
void   refreshSlabsData(Screen *screen, int mcConn);

#endif //MCADMIN_SCREENS_H
