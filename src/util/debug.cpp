#include "debug.h"

#include <iostream>

namespace util {
	void displayMessage(std::string msg, int type) {
		switch (type) {
		default:
		case INFO:
			std::cout << "INFO: " << msg << std::endl;
			break;
		case WARN:
			std::cout << "WARN: " << msg << std::endl;
			break;
		case ERR:
			std::cerr << "ERROR: " << msg << std::endl;
			break;
		}
	}

	void displayError(std::string err) {
		displayMessage(err, ERR);

		throw std::runtime_error(err);
	}
}