#include <stdio.h>
#include <ncurses.h>

int main() {

    WINDOW *win = NULL;
    const int height = 5;
    const int width = 21;

    int x = 0, y = 0;

    initscr();

    x = (COLS-width)/2;
    y = (LINES-height)/2;

    win = newwin(height, width, y, x);

    if (win == NULL) {
        return 1;
    }

    refresh();

    wborder(win, '|', '|', '-', '-', '+', '+', '+', '+');

    mvwprintw(win, 2, 4, "Hello, World!");
    
    refresh();
    wrefresh(win);
   
    refresh();

    getch();

    delwin(win);

    endwin();
    return 0;
}
