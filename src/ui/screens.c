#include "screens.h"

Stats stats;
Slab  slabs;
int   slabCount;

typedef struct Measurement Measurement;
struct Measurement {
    char *name;
    unsigned long size;
};

static Measurement byteMeasurements[] = {
    {"bytes", 1},
    {"KB",    1024},
    {"MB",    1024 * 1024},
    {"GB",    1024 * 1024 * 1024},
};

static Measurement timeMeasurements[] = {
    {"seconds", 1},
    {"minutes", 60},
    {"hours",   60 * 60},
    {"days",    24 * 60 * 60},
};

static Measurement
getMeasurement(long value, int count, Measurement measurements[count])
{
    for (int i = count - 1; i >= 0; --i) {
        if (value / measurements[i].size >= 1) {
            return measurements[i];
        }
    }

    return measurements[0];
}

static Measurement
getByteMeasurement(long bytes)
{
    return getMeasurement(bytes, sizeof(byteMeasurements) / sizeof(byteMeasurements[0]), byteMeasurements);
}

static Measurement
getTimeMeasurement(long time)
{
    return getMeasurement(time, sizeof(timeMeasurements) / sizeof(timeMeasurements[0]), timeMeasurements);
}

static void
connectPanels(int panelCount, PANEL *panels[panelCount])
{
    for (int i = 1; i < panelCount; ++i) {
        set_panel_userptr(panels[i - 1], panels[i]);
    }

    set_panel_userptr(panels[panelCount - 1], panels[0]);
}

Screen *
createScreen(
    int panelCount,
    PANEL *panels[panelCount],
    Screen *next,
    void (*refreshView)(Screen *)
)
{
    Screen *screen = malloc(sizeof(Screen));

    screen->panelCount = panelCount;
    screen->panels = panels;
    screen->currentPanel = panels[0];
    screen->refreshView = refreshView;
    screen->next = next;
    connectPanels(panelCount, screen->panels);

    return screen;
}

void
refreshStatsView(Screen *screen)
{
    int row = 0;

    mvwprintw(
            screen->currentPanel->win,
            row++,
            0,
            "Memcache %s                                                                                              ",
            stats.version
    );
    mvwprintw(
        screen->currentPanel->win,
        row++,
        0,
        "Process      PID: %ld, uptime: %.1f %s                                                                       ",
        stats.pid,

        (float) stats.uptime / getTimeMeasurement(stats.uptime).size,
        getTimeMeasurement(stats.uptime).name
    );
    mvwprintw(
        screen->currentPanel->win,
        row++,
        0,
        "CPU time     user: %.2f, system: %.2f",
        stats.rusage_user,
        stats.rusage_system
    );
    mvwprintw(
        screen->currentPanel->win,
        row++,
        0,
        "Memory       used: %.1f %s, available: %.1f %s                                                               ",

        (float) stats.bytes / getByteMeasurement(stats.bytes).size,
        getByteMeasurement(stats.bytes).name,

        (float) stats.limit_maxbytes / getByteMeasurement(stats.limit_maxbytes).size,
        getByteMeasurement(stats.limit_maxbytes).name
    );
    mvwprintw(
        screen->currentPanel->win,
        row++,
        0,
        "Network      read: %.1f %s, written: %.1f %s                                                                 ",

        (float) stats.bytes_read / getByteMeasurement(stats.bytes_read).size,
        getByteMeasurement(stats.bytes_read).name,

        (float) stats.bytes_written / getByteMeasurement(stats.bytes_written).size,
        getByteMeasurement(stats.bytes_written).name
    );
    mvwprintw(
        screen->currentPanel->win,
        row++,
        0,
        "Connections  current: %ld, total: %ld                                                                        ",
        stats.curr_connections,
        stats.total_connections
    );
    mvwprintw(
        screen->currentPanel->win,
        row++,
        0,
        "Commands     set: %ld, get: %ldh/%ldm, delete: %ldh/%ldm, cas: %ldh/%ldm/%ldb                                ",
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
        "             incr: %ldh/%ldm, decr: %ldh/%ldm, touch: %ldh/%ldm                                              ",
        stats.incr_hits,
        stats.incr_misses,
        stats.decr_hits,
        stats.decr_misses,
        stats.touch_hits,
        stats.touch_misses
    );
}

void
refreshSlabsView(Screen *screen)
{
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
    int  row = 0;
    Slab *current = &slabs;
    while (current) {
        row = 0;

        mvwprintw(
            screen->panels[i]->win,
            row++,
            0,
            "Slab %ld (%d of %d) | <TAB>: cycle through slabs                                                         ",
            current->class,
            i + 1,
            slabCount
        );
        mvwprintw(
            screen->panels[i]->win,
            row++,
            0,
            "Chunks  amount: %ld, used: %ld, free: %ld, size: %.1f %s                                                 ",
            current->total_chunks,
            current->used_chunks,
            current->free_chunks,

            (float) current->chunk_size / getByteMeasurement(current->chunk_size).size,
            getByteMeasurement(current->chunk_size).name
        );
        mvwprintw(
            screen->panels[i]->win,
            row++,
            0,
            "Pages   amount: %ld, chunks per page: %ld                                                                ",
            current->total_pages,
            current->chunks_per_page
        );

        long maxMemory = current->total_chunks * current->chunk_size;
        long freeMemory = maxMemory - current->mem_requested;

        mvwprintw(
            screen->panels[i]->win,
            row++,
            0,
            "Memory  amount: %.1f %s, used: %.1f %s, free: %.1f %s                                                    ",

            (float) maxMemory / getByteMeasurement(maxMemory).size,
            getByteMeasurement(maxMemory).name,

            (float) current->mem_requested / getByteMeasurement(current->mem_requested).size,
            getByteMeasurement(current->mem_requested).name,

            (float) freeMemory / getByteMeasurement(freeMemory).size,
            getByteMeasurement(freeMemory).name
        );

        current = current->next;
        ++i;
    }
}
