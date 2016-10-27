#ifndef MCADMIN_SCREENS_H
#define MCADMIN_SCREENS_H

#include <curses.h>
#include <malloc.h>
#include <panel.h>
#include "../memcache/commands.h"
#include "../memcache/stats.h"

typedef struct Screen Screen;
struct Screen {
    int    panelCount;
    PANEL  **panels;
    PANEL  *currentPanel;
    void   (*refreshView)(Screen *this);
    Screen *next;
};

Screen *createScreen(
    int panelCount,
    PANEL *panels[panelCount],
    Screen *next,
    void (*refreshView)(Screen *this)
);

void refreshStatsView(Screen *screen);
void refreshSlabsView(Screen *screen);

#endif //MCADMIN_SCREENS_H
