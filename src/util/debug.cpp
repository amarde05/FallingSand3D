#include "debug.h"

#include <iostream>

namespace util {
	void displayMessage(std::string msg, int type) {
		switch (type) {
		default:
		case DISPLAY_TYPE_INFO:
			std::cout << "INFO: " << msg << std::endl;
			break;
		case DISPLAY_TYPE_WARN:
			std::cout << "WARN: " << msg << std::endl;
			break;
		case DISPLAY_TYPE_ERR:
			std::cerr << "ERROR: " << msg << std::endl;
			break;
		case DISPLAY_TYPE_NONE:
			std::cout << msg << std::endl;
		}
	}

	void displayError(std::string err) {
		displayMessage(err, DISPLAY_TYPE_ERR);

		throw std::runtime_error(err);
	}
}