#include <ncurses.h>
#include <ncurses/panel.h>
#include <stdlib.h>

struct basicWindow {
	WINDOW *win;
	int lines, cols, y, x;
	basicWindow(int lines_, int cols_, int y_, int x_) :
			lines(lines_), cols(cols_), y(y_), x(x_) {
		win = newwin(lines, cols, y, x);
	}

	~basicWindow() {
		delwin(win);
	}
};

struct windowSession {
	WINDOW *mainWin;
	windowSession() {
		initscr();
		cbreak();
		noecho();
		keypad(stdscr, TRUE);
		curs_set(0);
		mousemask(BUTTON1_CLICKED, nullptr);
		initMainWindow();
	}
	void initMainWindow() {
		mainWin = newwin(LINES - 1, COLS - 1, 0, 0);
	}

	~windowSession() {
		endwin();
	}
};
