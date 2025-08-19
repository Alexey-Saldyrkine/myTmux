#pragma once
#include "myWindows.hpp"
#include <stdlib.h>
#include <fcntl.h>
#include <system_error>
#include <unistd.h>
#include <iostream>
#include <signal.h>
#include <termios.h>
#include <array>
#include "ANSI_Parser.hpp"
constexpr const size_t termBuffSize = 4096;

struct ptyManager {
	int parentFd;
	int childFd = -1;
	int childPid = -1;
	std::string nameStr;
	bool isParent = false;
	ptyManager() {
		parentFd = posix_openpt(O_RDWR | O_NOCTTY);
		if (parentFd < 0) {
			throw std::system_error(errno, std::generic_category(),
					"posix_openat failed");
		}
		int ret2 = unlockpt(parentFd);
		if (ret2 != 0) {
			close(parentFd);
			throw std::system_error(errno, std::generic_category(),
					"unlockpt failed");
		}
		int ret1 = grantpt(parentFd);
		if (ret1 != 0) {
			close(parentFd);
			throw std::system_error(errno, std::generic_category(),
					"grantpt failed");
		}

		char *name = ptsname(parentFd);
		if (name == nullptr) {
			close(parentFd);
			throw std::system_error(errno, std::generic_category(),
					"ptsname failed");
		}
		nameStr = std::string { name };

		struct termios setting;
		tcgetattr(parentFd, &setting);
		cfmakeraw(&setting);
		setting.c_lflag |= (ECHO | ECHOE | ECHOK | ECHONL);
		tcsetattr(parentFd, TCSANOW, &setting);
		fcntl(parentFd, F_SETFL, FNDELAY);
		/*childFd = open(name, O_RDWR | O_NOCTTY);
		 if (childFd < 0) {
		 close(parentFd);
		 throw std::system_error(errno, std::generic_category(),
		 "open tty failed");
		 }*/

		/*std::string xterm_cmd { "xterm" };
		 xterm_cmd += " -S"+nameStr + "/" + std::to_string(parentFd);
		 //xterm_cmd += " -e $SHELL -c \'echo $TERM \'";
		 std::cout << xterm_cmd << std::endl;
		 FILE *xterm_stdout = popen(xterm_cmd.c_str(), "r");
		 if (xterm_stdout <= 0) {
		 close(parentFd);
		 close(childFd);
		 throw std::system_error(errno, std::generic_category(),
		 "open xterm failed");
		 }*/

	}
	~ptyManager() {
		if (childPid != -1) {
			kill(childPid, SIGTERM);
		}
		if (parentFd >= 0) {
			close(parentFd);
		}
		if (childFd >= 0) {
			close(childFd);
		}
	}

	bool forkPty() {
		childPid = fork();
		if (childPid == 0) {
			//setsid();
			int childFd = open(nameStr.c_str(), O_RDWR);

			dup2(childFd, 0);
			dup2(childFd, 1);
			dup2(childFd, 2);
			//std::string resizeCmd { "resize -s " };
			//resizeCmd += std::to_string(y) + " " + std::to_string(x);
			const char *args[] = { "/bin/bash" "-i", NULL };
			execv("/bin/bash", (char* const*) args);
			exit(101);
//			std::string cmd { "xterm -S" };
//			cmd += nameStr.substr(nameStr.find_last_of("/") + 1,
//					nameStr.size());
//			cmd += "/" + std::to_string(childFd);
//			cmd += " -e $SHELL";
			//system(cmd.c_str());
			return true;
		} else {
			isParent = true;
			return false;
		}
	}

	void sendResize(int lines, int cols) {
		auto str = resizeCommandString(lines, cols);
		write(parentFd, str.c_str(), str.size());
	}

	void sendResizeStart(int lines, int cols) {
		//auto str = "printf " + resizeCommandString(lines, cols) + "\n";
		std::string str = "export LINES=" + std::to_string(lines)
				+ "\nexport COLUMNS=" + std::to_string(cols) + "\n";
		write(parentFd, str.c_str(), str.size());
	}

	void send(int c) {
		write(parentFd, &c, sizeof(c));
	}

	std::string readLine() {
		std::string str;
		std::array<char, termBuffSize> buff { };
		size_t size = 0;
		//std::cout << "[begin read]\n";

		while (int n = read(parentFd, buff.data(), termBuffSize) > 0) {
			//std::cout << "[n = " << n << "]\n";
			if (n > 0) {
				std::string msg(buff.data());
				buff.fill(0);
				//std::cout << "[msg:" << msg << "]\n";
				str += msg;
				size += n;
			}
		}
		//std::cout << "[message size: " << size << "]\n";
		return str;
	}
};

// todo proper resize move for term buffers !
struct emptyScreen: screenBase {
	terminalData &term;
	emptyScreen(terminalData &t, WINDOW *win, button_interface *buttonPtr =
			nullptr) :
			screenBase(new_panel(win)), term(t) {
		setButtonInterfaceUsrPtr(panel, buttonPtr);
		hide();
	}
	emptyScreen(const emptyScreen &other) = delete;
	emptyScreen(emptyScreen &&other) = delete;
	emptyScreen& operator=(const emptyScreen &other) = delete;
	emptyScreen& operator=(emptyScreen &&other) = delete;

	~emptyScreen() {
		//destroy_panel(panel);
	}

	void setWindow(WINDOW *win) override {
		replace_panel(panel, win);
	}

	operator WINDOW*() override {
		return panel_window(panel);
	}

	void resizeMove(const screenArea &area) override {
		//win = basicWindow(area);
		//replace_panel(panel, win.win);
		term.moveResize(area);
		setWindow(term.getBuff().win);
	}
	void move(int y, int x) override {
		move_panel(panel, y, x);
	}
	void hide() override {
		hide_panel(panel);
	}
	void show() override {
		show_panel(panel);
	}
	void highlight() override {

	}
	void lowlight() override {

	}
};

struct terminalScreen: button_interface {
	std::unique_ptr<borderedScreen> textscreen;
	std::string str;
	ptyManager manager;
	terminalData term;
	terminalScreen(const screenArea &area) :
			term(area) {
		textscreen = std::make_unique<borderedScreen>(area,
				std::make_unique<emptyScreen>(term, term.mainBuffer.win,
						static_cast<button_interface*>(this)));
		printX();
		manager.forkPty();
		manager.sendResizeStart(area.lines - 2, area.cols - 2);
		idlok(*textscreen->innerScreen, true);
		scrollok(*textscreen->innerScreen, true);
	}

	void printX() {
		mvwprintw(textscreen->win, 0, 0, "[X]");
	}
	void print() {
		//wclear(*textscreen->innerScreen);
		if (str.size() > 0) {
			ansiwPrint(term, str.c_str());
			str.clear();
			textscreen->innerScreen->setWindow(term.getBuff().win);
		}

	}

	bool processKey(int c, PANEL *&focusPanel) override {
		//if (c <= 255) {
		//	char str[2] = { };
		//str[0] = static_cast<char>(c);
		//wprintw(*textscreen->innerScreen, str);
		//}
		switch (c) {
		case KEY_BACKSPACE:
			manager.send(0x8);
			break;
		default:
			manager.send(c);
			break;
		}

		tick();
		return true;
	}
	bool processWindowResize(const MEVENT &event, screen *thisScreen) {
		WINDOW *win = thisScreen->win.win;
		if (resizeMode) {
			resizeMode = false;
			if ((event.bstate & BUTTON1_RELEASED)) {
				auto area = calcResizeArea(win, prevY, prevX, event.y, event.x);
				manager.sendResize(area.lines, area.cols);
				thisScreen->resizeMove(area);
				return true;
			}
		}
		if ((event.bstate & BUTTON1_PRESSED)) {
			if (isBottomRightEdge(win, event.y, event.x)) {
				resizeMode = true;
				prevX = event.x;
				prevY = event.y;
			}
		}
		return false;
	}

	bool processMouseEvent(const MEVENT &event, PANEL *&focusPanel) override {
		highlight();
		hideWindow();
		showWindow();

		processWindowMove(event, textscreen.get());
		processWindowResize(event, textscreen.get());
		return true;
	}

	void highlight() override {
		textscreen->highlight();
		printX();
	}
	void lowlight() override {
		textscreen->lowlight();
		printX();
	}
	void showWindow() override {
		textscreen->show();
	}
	void hideWindow() override {
		textscreen->hide();
	}

	void tick() override {
		auto nStr = manager.readLine();
		if (nStr.size() > 0) {
			str = nStr;
		}
		print();
	}

}
;
