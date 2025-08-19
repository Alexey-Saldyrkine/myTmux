#pragma once
#include <vector>

constexpr char BLANKCHAR = ' ';

constexpr bool logCommands = true;

template<typename T>
struct processItem {
	static T f(T &&Arg) {
		return std::forward<T>(Arg);
	}
};

template<size_t N>
struct processItem<const char[N]> {
	static std::string f(const char str[N]) {
		return std::string(str, N);
	}
};

template<typename ... T>
void logItems(T &&... Args) {
	if constexpr (logCommands) {
		(std::cerr<<...<<Args);
		std::cerr << std::endl;
	}
}

template<bool>
struct CSI_string {
	void setStr(const std::string &vstr) {

	}
	std::string getStr() const {
		return "";
	}
};

template<>
struct CSI_string<true> {
	std::string str;

	void setStr(const std::string &vstr) {
		str = vstr;
	}
	std::string getStr() const {
		return str;
	}
};

struct CSI: CSI_string<logCommands> {
	int args[4] = { };
	int argc = 0;
	char funcName = 0;
};

std::string CSItoString(const CSI &data, bool flag = true) {
	std::string str = "";
	if (flag) {
		str += "[CSI name:";
	} else {
		str += "[CSI? name:";
	}
	str += data.funcName;
	str += " argc:" + std::to_string(data.argc) + " args:";
	for (int i = 0; i < data.argc; i++) {
		str += std::to_string(data.args[i]) + ", ";
	}
	if constexpr (logCommands) {
		str += " str:\"CSI" + data.getStr() + "\"";
	}
	str += "]";
	return str;
}

struct terminalBuffer {
	basicWindow win;
	int savedX = 0, savedY = 0;
	attr_t savedAttr = 0;
	short int savedColor = 0;

	terminalBuffer(screenArea area) :
			win(area) {
	}

	void saveState() {
		getyx(win, savedY, savedX);
		wattr_get(win, &savedAttr, &savedColor, nullptr);
	}
	void restoreState() {
		wmove(win, savedY, savedX);
		wattr_set(win, savedAttr, savedColor, nullptr);
	}

	void resize(screenArea area) {
		auto nw = basicWindow(area);
		int minRow = std::min(win.lines, nw.lines);
		int minCol = std::min(win.cols, nw.cols);
		copywin(win, nw, 0, 0, 0, 0, minRow, minCol, false);
		win = nw;
	}
};

struct terminalData {
	terminalBuffer mainBuffer;
	terminalBuffer alternateBuffer;
	bool usingMain = true;

	terminalData(screenArea area) :
			mainBuffer(area), alternateBuffer(area) {
	}

	void switchToAlt() {
		usingMain = false;
	}

	void switchToMain() {
		usingMain = true;
	}

	terminalBuffer& getBuff() {
		if (usingMain) {
			return mainBuffer;
		} else {
			return alternateBuffer;
		}
	}

	void moveResize(screenArea area) {
		mainBuffer.resize(area);
		alternateBuffer.resize(area);
	}
};

bool isC1_8bit(const char *ptr) {
	switch (static_cast<unsigned char>(*ptr)) {
	case 0x84:
	case 0x85:
	case 0x88:
	case 0x8d:
	case 0x8e:
	case 0x8f:
	case 0x90:
	case 0x96:
	case 0x97:
	case 0x98:
	case 0x9a:
	case 0x9b:
	case 0x9c:
	case 0x9d:
	case 0x9e:
	case 0x9f:
		return true;
		break;
	default:
		return false;
		break;
	}
}

bool isCSI(const char *ptr) {
	return (ptr[0] == 0x1b && ptr[1] == '[')
			|| static_cast<unsigned char>(ptr[0] == 0x9b);
}

bool isLetter(const char c) {
	return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

bool isNumber(const char c) {
	return '0' <= c && c <= '9';
}

CSI parseCSI(const char *&ptr) {
	const char *start = ptr;
	CSI ret;
	std::string num;
	while (!isLetter(*ptr)) {
		if (isNumber(*ptr)) {
			num += *ptr;
		} else if (*ptr == ';') {
			if (num.size() > 0) {
				ret.args[ret.argc++] = std::stoi(num);
			} else {
				ret.args[ret.argc++] = 1;
			}
			num.clear();
		}
		ptr++;
	}
	if (num.size() > 0) {
		ret.args[ret.argc++] = std::stoi(num);
	}
	ret.funcName = *ptr;

	const char *end = ptr;
	if constexpr (logCommands) {
		ret.setStr(std::string(start + 2, end - start - 1));
	}

	return ret;
}

void SCI_move_Cursor(WINDOW *win, const CSI &data) {
	if (data.funcName == 'H') {
		int y = 0, x = 0;
		if (data.argc > 0) {
			y = data.args[0] - 1;
		}
		if (data.argc > 1) {
			x = data.args[1] - 1;
		}
		wmove(win, y, x);
	} else {
		int y, x;
		getyx(win, y, x);
		int d = 1;
		if (data.argc > 0) {
			d = data.args[0];
		}
		switch (data.funcName) {
		case 'A':
			y -= d;
			break;
		case 'B':
			y += d;
			break;
		case 'C':
			x += d;
			break;
		case 'D':
			x -= d;
			break;
		case 'E':
			y += d;
			x = 0;
			break;
		case 'F':
			y -= d;
			x = 0;
			break;
		case 'G':
			x = d;
			break;
		default:
			logItems("SCI_move_cursor ", CSItoString(data), " <-- unknowen !!");
			return;
			break;
		}
		wmove(win, y, x);
	}

}

void setAttributes(WINDOW *win, int arg, const CSI &data) {
	switch (arg) {
	case 0:
		wattrset(win, A_NORMAL);
		break;
	case 1:
		wattron(win, A_BOLD);
		break;
	case 2:
		wattron(win, A_DIM);
		break;
	case 3:
		wattron(win, A_ITALIC);
		break;
	case 4:
		wattron(win, A_UNDERLINE);
		break;
	case 5:
		wattron(win, A_BLINK);
		break;
	case 7:
		wattron(win, A_REVERSE);
		break;
	case 8:
		wattron(win, A_INVIS);
		break;
	case 21:
	case 22:
		wattroff(win, A_BOLD);
		break;
	case 23:
		wattroff(win, A_ITALIC);
		break;
	case 24:
		wattroff(win, A_UNDERLINE);
		break;
	case 25:
		wattroff(win, A_BLINK);
		break;
	case 27:
		wattroff(win, A_REVERSE);
		break;
	case 28:
		wattroff(win, A_INVIS);
		break;
	default:
		logItems("setAttributes ", CSItoString(data), " <- unkowen !!");
		break;
	}
}

void setColors(WINDOW *win, int arg0, const CSI &data) {
	switch (arg0) {
	case 30:
		setFgColor(win, COLOR_BLACK);
		break;
	case 31:
		setFgColor(win, COLOR_RED);
		break;
	case 32:
		setFgColor(win, COLOR_GREEN);
		break;
	case 33:
		setFgColor(win, COLOR_YELLOW);
		break;
	case 34:
		setFgColor(win, COLOR_BLUE);
		break;
	case 35:
		setFgColor(win, COLOR_MAGENTA);
		break;
	case 36:
		setFgColor(win, COLOR_CYAN);
		break;
	case 37:
		setFgColor(win, COLOR_WHITE);
		break;
	case 39:
		setBgColor(win, 99);
		break;
	case 40:
		setBgColor(win, COLOR_BLACK);
		break;
	case 41:
		setBgColor(win, COLOR_RED);
		break;
	case 42:
		setBgColor(win, COLOR_GREEN);
		break;
	case 43:
		setBgColor(win, COLOR_YELLOW);
		break;
	case 44:
		setBgColor(win, COLOR_BLUE);
		break;
	case 45:
		setBgColor(win, COLOR_MAGENTA);
		break;
	case 46:
		setBgColor(win, COLOR_CYAN);
		break;
	case 47:
		setBgColor(win, COLOR_WHITE);
		break;
	case 49:
		setBgColor(win, 99);
		break;
	case 90:
		setFgColorBright(win, COLOR_BLACK);
		break;
	case 91:
		setFgColorBright(win, COLOR_RED);
		break;
	case 92:
		setFgColorBright(win, COLOR_GREEN);
		break;
	case 93:
		setFgColorBright(win, COLOR_YELLOW);
		break;
	case 94:
		setFgColorBright(win, COLOR_BLUE);
		break;
	case 95:
		setFgColorBright(win, COLOR_MAGENTA);
		break;
	case 96:
		setFgColorBright(win, COLOR_CYAN);
		break;
	case 97:
		setFgColorBright(win, COLOR_WHITE);
		break;
	case 100:
		setBgColorBright(win, COLOR_BLACK);
		break;
	case 101:
		setBgColorBright(win, COLOR_RED);
		break;
	case 102:
		setBgColorBright(win, COLOR_GREEN);
		break;
	case 103:
		setBgColorBright(win, COLOR_YELLOW);
		break;
	case 104:
		setBgColorBright(win, COLOR_BLUE);
		break;
	case 105:
		setBgColorBright(win, COLOR_MAGENTA);
		break;
	case 106:
		setBgColorBright(win, COLOR_CYAN);
		break;
	case 107:
		setBgColorBright(win, COLOR_WHITE);
		break;
	default:
		logItems("setColors ", CSItoString(data), " <-- unkowen !!");
		break;
	}
}

void CSI_GRP(WINDOW *win, const CSI &data) {
	if (data.argc == 0) {
		wattrset(win, A_NORMAL);
	} else {
		for (int i = 0; i < data.argc; i++) {
			int arg = data.args[i];
			if (arg <= 28) {
				setAttributes(win, arg, data);
			} else if ((30 <= arg && arg <= 49) || (90 <= arg && arg <= 1070)) {
				setColors(win, arg, data);
			} else {
				logItems("CSI_GRP ", std::to_string(arg), " <-- unkowen !!");
			}
		}
	}
}

void clearWindowAbove(WINDOW *win) {
	int y, x;
	getyx(win, y, x);
	for (int i = 0; i < y; i++) {
		wmove(win, i, 0);
		wclrtoeol(win);
	}
	wmove(win, y, 0);
	wprintw(win, std::string(x, BLANKCHAR).c_str());
	wmove(win, x, y);
}
void clearLineLeft(WINDOW *win) {
	int y, x;
	getyx(win, y, x);
	wmove(win, y, 0);
	wprintw(win, std::string(x, BLANKCHAR).c_str());
	wmove(win, x, y);
}

void CSI_J(WINDOW *win, const CSI &data) {
	switch (data.args[0]) {
	case 0:
		wclrtobot(win);
		break;
	case 1:
		clearWindowAbove(win);
		break;
	case 2:
		werase(win);
		break;
	case 3:
		wclear(win);
		break;
	default:
		logItems("CSI_J", CSItoString(data), " <-- unkowen!!");
		break;
	}
}

void CSI_K(WINDOW *win, const CSI &data) {
	//printCSIData(win, data);
	switch (data.args[0]) {
	case 0:
		wclrtoeol(win);
		break;
	case 1:
		clearLineLeft(win);
		break;
	case 2:
		int y, x;
		getyx(win, y, x);
		wmove(win, y, 0);
		wclrtoeol(win);
		wmove(win, y, x);
		break;
	default:
		logItems("CSI_K", CSItoString(data), " p = ", data.args[0],
				" <-- unkowen!!");
		break;
	}
}

void CSI_d(WINDOW *win, const CSI &data) {
	int y, x;
	getyx(win, y, x);
	if (data.argc == 0) {
		y = 0;
	} else if (data.argc == 1) {
		y = data.args[0] - 1;
	} else if (data.argc == 2) {
		y = data.args[0] - 1;
		x = data.args[1] - 1;
	} else {
		logItems("CSI_d", CSItoString(data), " <-- unkowen!!");
		return;
	}
	wmove(win, y, x);
}
void CSI_X(WINDOW *win, const CSI &data) {
	int n = 1;
	if (data.argc == 1) {
		n = data.args[0];
	} else if (data.argc != 0) {
		logItems("CSI_X", CSItoString(data), " <-- unkowen!!");
		return;
	}
	int y, x, mx;
	getyx(win, y, x);
	mx = getmaxy(win);
	int i = x + y * mx;
	int ei = i - n;
	int ey = ei / mx;
	int ex = ei % mx;
	wmove(win, ey, ex);
	wprintw(win, std::string(n, BLANKCHAR).c_str());
	wmove(win, ey, ex);
}

//void CSI_t(WINDOW *win, const CSI &data) {
//
//}

void CSI_ALT_h(terminalData &term, const CSI &data) {
	for (int i = 0; i < data.argc; i++) {
		switch (data.args[i]) {
		case 1:
			logItems("CSI_ALT_h ", 1, " <-- ignored");
			break;
		case 7:
			logItems("CSI_ALT_h ", 7, " <-- ignored");
			break;
		case 25:
			logItems("CSI_ALT_h ", 25, " <-- ignored");
			break;
		case 1000:
			logItems("CSI_ALT_h ", 1000, " <-- ignored");
			break;
		case 1006:
			logItems("CSI_ALT_h ", 1006, " <-- ignored");
			break;
		case 1047:
			term.switchToAlt();
			break;
		case 1048:
			term.getBuff().saveState();
			break;
		case 1049:
			term.getBuff().saveState();
			term.switchToAlt();
			break;
		default:
			logItems("CSI_ALT_h", CSItoString(data), " p = ", data.args[i],
					" <-- unkowen!!");
			break;
		}
	}
}

void CSI_ALT_l(terminalData &term, const CSI &data) {
	for (int i = 0; i < data.argc; i++) {
		switch (data.args[i]) {
		case 1:
			logItems("CSI_ALT_h ", 1, " <-- ignored");
			break;
		case 7:
			logItems("CSI_ALT_h ", 7, " <-- ignored");
			break;
		case 25:
			logItems("CSI_ALT_h ", 25, " <-- ignored");
			break;
		case 1000:
			logItems("CSI_ALT_h ", 1000, " <-- ignored");
			break;
		case 1006:
			logItems("CSI_ALT_h ", 1006, " <-- ignored");
			break;
		case 1047:
			term.switchToMain();
			break;
		case 1048:
			term.getBuff().restoreState();
			break;
		case 1049:
			term.switchToMain();
			term.getBuff().restoreState();
			break;
		default:
			logItems("CSI_ALT_l", CSItoString(data), " p = ", data.args[i],
					" <-- unkowen!!");
			break;
		}
	}
}

void CSI_r(WINDOW *win, const CSI &data) {
	if (data.argc == 2) {
		wsetscrreg(win, data.args[0], data.args[1]);
	} else if (data.argc == 0) {
		wsetscrreg(win, 0, getmaxy(win));
	} else {
		logItems("CSI_r", CSItoString(data), " <-- unkowen!!");
	}
}

char getPrevChar(WINDOW *win) {
	int x, y;
	getyx(win, y, x);
	int ex = x, ey = y;
	int mx = getmaxy(win);
	if (x == 0) {
		y--;
		x = mx - 1;
	} else {
		x--;
	}
	if (y >= 0) {
		auto c = mvwinch(win, y, x);
		char ret = static_cast<char>(c);
		wmove(win, ey, ex);
		//std::cerr << "bx = " << ex << ", by = " << ey << ", x = " << x
		//		<< ", y = " << y << ", c = " << ret << "]" << std::endl;
		return ret;
	} else {
		return ' ';
	}
}

void CSI_b(WINDOW *win, const CSI &data) {
	int n = data.args[0];
	if (n == 0)
		n = 1;
	std::string str(n, getPrevChar(win));
	logItems("CSI_b", CSItoString(data), "prev char: \"", str[0], "\"");
	wprintw(win, str.c_str());
}

void CSI_P(WINDOW *win, const CSI &data) {
	int n = data.args[0];
	if (data.argc == 0) {
		n = 1;
	}
	std::string str(n, 8);
	logItems("CSI_P", CSItoString(data), " deleting ", n, " characters");
	wprintw(win, str.c_str());
}

const char* processAltCSI(terminalData &term, const char *ptr) {
	CSI data = parseCSI(ptr);
	switch (data.funcName) {
	case 'h':
		CSI_ALT_h(term, data);
		break;
	case 'l':
		CSI_ALT_l(term, data);
		break;
	case 's':
	case 'r':
		// same as h, apparently
		CSI_ALT_h(term, data);
		break;
	default:
		logItems("processAltCSI", CSItoString(data), " <-- unkowen!!");
		break;
	}
	return ptr;
}

const char* processContolSequence(terminalData &term, const char *ptr) {
	WINDOW *win = term.getBuff().win;
	if (ptr[2] == '?') {
		ptr = processAltCSI(term, ptr);
		return ptr;
	} else {
		CSI data = parseCSI(ptr);
		if ('A' <= data.funcName && data.funcName <= 'H') {
			SCI_move_Cursor(win, data);
		} else {
			switch (data.funcName) {
			case 'b':
				CSI_b(win, data);
				break;
			case 'd':
				CSI_d(win, data);
				break;
			case 'l':
				logItems("CSI_l", CSItoString(data), " <-- Ignored");
				//ignore
				break;
			case 'm':
				CSI_GRP(win, data);
				break;
			case 'r':
				CSI_r(win, data);
				break;
			case 't':
				logItems("CSI_t", CSItoString(data), " <-- Ignored");
				//ignore
				break;
			case 'P':
				CSI_P(win, data);
				break;
			case 'J':
				CSI_J(win, data);
				break;
			case 'K':
				CSI_K(win, data);
				break;
			case 'X':
				CSI_X(win, data);
				break;
			default:
				logItems("processContolSequence", CSItoString(data),
						" <-- unkowen!!");
				break;
			}
		}
		return ptr;
	}
}

bool isOSC(const char *ptr) {
	return (ptr[0] == 0x1b && ptr[1] == ']')
			|| static_cast<unsigned char>(ptr[0]) == 0x9d;
}

bool isST(const char *ptr) {
	return (ptr[0] == 0x1b && ptr[1] == '\\')
			|| static_cast<unsigned char>(ptr[0]) == 0x9c;
}

const char* gotoEndOfST(const char *ptr) {
	if (static_cast<unsigned char>(ptr[0]) == 0x9c) {
		ptr++;
	} else {
		ptr += 2;
	}
	return ptr;
}

//struct OSC{
//	int code;
//	std::string str;
//
//};

int getOSCcode(const char *ptr) {
	std::string num;
	while (*ptr != ';') {
		if (isNumber(*ptr))
			num += *(ptr);
		ptr++;
	}
	ptr++;
	if (num.size() > 0) {
		return std::stoi(num);
	} else {
		return 0;
	}
}

std::string getOSCstring(const char *&ptr) {
	std::string ret;
	while (*ptr != 0x7 && !isST(ptr)) {
		ret += *(ptr++);
	}
	ptr++;
	return ret;
}

void skipOSCstring(const char *&ptr) {
	while (*ptr != 0x7 && !isST(ptr)) {
		ptr++;
	}
	ptr++;
}

const char* OSCskip_1(const char *&ptr) {
	skipOSCstring(ptr);
	return ptr;
}

bool skipOSC(const char *&ptr) {
	while (!(*ptr == 0x7 || isST(ptr))) {
		ptr++;
	}
	if (ptr[0] == 0x1b) {
		ptr++;
		return true;
	} else {
		return false;
	}
}

const char* processOperatingSystemControl(terminalData &term, const char *ptr) {
	const char *beg = ptr;
	int code = getOSCcode(ptr);
	int i = skipOSC(ptr);
	switch (code) {
//	case 0:
//	case 1:
//	case 2:
//	case 7:
//		break;
	default:
		logItems("processOSC \"", std::string(beg + 2, ptr - beg - 2 - i),
				"\" <-- unkowen!!");
		break;
	}
	return ptr;
}

const char* processC1_8bit(const char *ptr) {
	int value = static_cast<int>(static_cast<unsigned char>(*ptr));
	switch (value) {
	default:
		logItems("C1 8bit code: 0x", std::hex, value, std::dec, " <-- unused");
		break;
	}
	return ptr + 1;
}

//bool isDCS(const char *ptr) {
//	return (ptr[0] == 0x1b && ptr[1] == 'P')
//			|| static_cast<unsigned char>(ptr[0]) == 0x90;
//}
//
//const char* endOfDCS(const char *ptr) {
//	while (!(isST(ptr))) {
//		ptr++;
//	}
//	if (isST(ptr)) {
//		ptr = gotoEndOfST(ptr);
//	} else {
//		ptr--;
//	}
//	return ptr;
//}
//
//const char* processDCS(const char *ptr) {
//	const char *start = ptr;
//	const char *end = endOfDCS(ptr);
//	if constexpr (logCommands) {
//		std::string DCSstr(start, end - start);
//		std::string str = "[DCS str:\"" + DCSstr + "\"]";
//		std::cerr << str << std::endl;
//	}
//	return end;
//}

const char* processMiscANSI(terminalData &term, const char *ptr) {
	if (ptr[0] != 0x1b) {
		return nullptr;
	} else {
		switch (ptr[1]) {
		case '7':
			term.getBuff().saveState();
			return ptr + 2;
			break;
		case '8':
			term.getBuff().restoreState();
			return ptr + 2;
			break;
		case '(':
			//ignore
			return ptr + 2;
			break;
		case '=':
		case '>':
			//ignore
			return ptr + 1;
			break;
		case '[':
		case ']':
			return nullptr;
			break;
		default:
			logItems("processMiscANSI ", ptr[1], " <-- unkowen !!");
			return nullptr;
			break;
		}
	}
}

void ansiwPrint(terminalData &term, const char *ptr) {
//wmove(win, 0, 0);
	while (*ptr != 0) {
		if constexpr (1) {
			const char *retPtr = processMiscANSI(term, ptr);
			if (retPtr != nullptr) {
				ptr = retPtr;
			} else if (isCSI(ptr)) {
				ptr = processContolSequence(term, ptr);
			} else if (isOSC(ptr)) {
				ptr = processOperatingSystemControl(term, ptr);
			}
			//else if (0 && isDCS(ptr)) {
			//	ptr = processDCS(ptr);
			//}
			else if (isC1_8bit(ptr)) {
				ptr = processC1_8bit(ptr);
			} else {
				waddch(term.getBuff().win, *ptr);
			}
		} else {
			std::cerr << *ptr;
			waddch(term.getBuff().win, *ptr);
		}
		ptr++;
	}
}

std::string resizeCommandString(int lines, int cols) {
	std::string ret = std::string(1, 0x1b) + "[8;" + std::to_string(lines) + ";"
			+ std::to_string(cols) + "t";
	return ret;
}
