#include "myBoxes.hpp"

int main() {
	windowManager manager;
	//manager.addWindow(
	//		std::make_unique<terminalScreen>(screenArea { 3, 3, 20, 70 }));
	//manager.addWindow(std::make_unique<textBox>(screenArea { 23, 2, 10, 30 }));
	//manager.addWindow(std::make_unique<textBox>(screenArea { 2, 83, 20, 20 }));
	//manager.addWindow(std::make_unique<colorShowcaseBox>(2, 75));
	//manager.addWindow(std::make_unique<tetrisBox>(2, 10));
	update_panels();
	doupdate();

	while (manager.isActive()) {
		manager.tick();
	}

	return 0;
}
