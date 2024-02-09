#pragma once

#include <string>

#define INFO 0
#define WARN 1
#define ERR 2

namespace util {
	void displayMessage(std::string msg, int type);

	void displayError(std::string err);
}