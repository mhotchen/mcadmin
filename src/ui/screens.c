#include "screens.h"

static void
connectPanels(int panelCount, PANEL *panels[panelCount])
{
    for (int i = 1; i < panelCount; ++i) {
        set_panel_userptr(panels[i - 1], panels[i]);
    }
    set_panel_userptr(panels[panelCount - 1], panels[0]);
}

Screen *
createScreen(int panelCount, PANEL *panels[panelCount], Screen *next, void (*refreshData)(Screen *, int))
{
    Screen *screen = malloc(sizeof(Screen));

    screen->panelCount = panelCount;
    screen->panels = panels;
    screen->currentPanel = panels[0];
    screen->refreshData = refreshData;
    screen->next = next;
    connectPanels(panelCount, screen->panels);

    return screen;
}

void
refreshStatsData(Screen *screen, int mcConn)
{
    Stats stats = {0};
    int row = 0;

    getStats(&stats, mcConn);

    mvwprintw(
        screen->currentPanel->win,
        row++,
        0,
        "Memcache %s, PID: %ld, uptime: %ld",
        stats.version,
        stats.pid,
        stats.uptime
    );
    mvwprintw(
        screen->currentPanel->win,
        row++,
        0,
        "CPU time: %.2f (usr), %.2f (sys)",
        stats.rusage_user,
        stats.rusage_system
    );
    mvwprintw(
        screen->currentPanel->win,
        row++,
        0,
        "Memory: used: %ld bytes, available: %ld bytes",
        stats.bytes,
        stats.limit_maxbytes
    );
    mvwprintw(
        screen->currentPanel->win,
        row++,
        0,
        "Network: read: %ld bytes, written: %ld bytes",
        stats.bytes_read,
        stats.bytes_written
    );
    mvwprintw(
        screen->currentPanel->win,
        row++,
        0,
        "Connections: %ld (curr), %ld (tot)",
        stats.curr_connections,
        stats.total_connections
    );
    mvwprintw(
        screen->currentPanel->win,
        row++,
        0,
        "Commands: set: %ld, get: %ldh/%ldm, delete %ldh/%ldm, cas %ldh/%ldm/%ldb",
        stats.cmd_set,
        stats.get_hits,
        stats.get_misses,
        stats.delete_hits,
        stats.delete_misses,
        stats.cas_hits,
        stats.cas_misses,
        stats.cas_badval
    );
    mvwprintw(
        screen->currentPanel->win,
        row++,
        0,
        "          incr: %ldh/%ldm, decr %ldh/%ldm, touch %ldh/%ldm",
        stats.incr_hits,
        stats.incr_misses,
        stats.decr_hits,
        stats.decr_misses,
        stats.touch_hits,
        stats.touch_misses
    );
}

void
refreshSlabsData(Screen *screen, int mcConn)
{
    Slab slab = {0};
    int row = 0;
    int slabCount = getSlabs(&slab, mcConn);

    if (slabCount != screen->panelCount) {
        int x, y, w, h;
        getbegyx(screen->currentPanel->win, y, x);
        getmaxyx(screen->currentPanel->win, h, w);
        for (int i = 0; i < screen->panelCount; ++i) {
            del_panel(screen->panels[i]);
        }
        screen->panelCount = slabCount;
        screen->panels = calloc(slabCount, sizeof(PANEL));
        for (int i = 0; i < slabCount; ++i) {
            screen->panels[i] = new_panel(newwin(h, w, y, x));
        }
        screen->currentPanel = screen->panels[0];
        connectPanels(screen->panelCount, screen->panels);
    }

    int i = 0;
    Slab *current = &slab;
    while (current) {
        row = 0;

        mvwprintw(
            screen->panels[i]->win,
            row++,
            0,
            "Slab %ld (%d of %d) | <TAB>: cycle through slabs",
            current->class,
            i + 1,
            slabCount
        );
        mvwprintw(
            screen->panels[i]->win,
            row++,
            0,
            "Chunks: amount: %ld, used: %ld, free: %ld, chunk size: %ld",
            current->total_chunks,
            current->used_chunks,
            current->free_chunks,
            current->chunk_size
        );
        mvwprintw(
            screen->panels[i]->win,
            row++,
            0,
            "Pages: amount: %ld, chunks per page: %ld",
            current->total_pages,
            current->chunks_per_page
        );
        mvwprintw(
            screen->panels[i]->win,
            row++,
            0,
            "Memory: max: %ld, used: %ld, free: %ld",
            current->total_chunks * current->chunk_size,
            current->mem_requested,
            (current->total_chunks * current->chunk_size) - current->mem_requested
        );

        current = current->next;
        ++i;
    }
}
