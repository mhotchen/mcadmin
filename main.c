#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include "memcache/connect.h"
#include "memcache/stats.h"

void usage(const char *const filename);
void showStats(WINDOW *const win, const stats *const s);

int
main(const int argc, const char *const argv[argc])
{
    if (argc != 3) {
        usage(argv[0]);
    }

    int sockfd = connectByNetworkSocket(argv[1], argv[2]);
    stats s = {0};

    initscr();
    cbreak();
    noecho();
    halfdelay(1);
    curs_set(0);

    WINDOW *const statsWin = newwin(LINES, COLS / 2, 0, COLS / 2);
    WINDOW *const menuWin = newwin(LINES, COLS / 2, 0, 0);

    bool quit = false;
    int command = 0;

    while (!quit) {
        getStats(&s, sockfd);

        box(statsWin, 0, 0);
        showStats(statsWin, &s);
        wrefresh(statsWin);

        box(menuWin, 0, 0);
        mvwprintw(menuWin, 1, 1, "Press q to quit");
        wrefresh(menuWin);

        refresh();
        command = getch();
        if (command == (int) 'q') {
            quit = true;
        }
    }

    endwin();

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
