#pragma once

#include <string>

#define DISPLAY_TYPE_INFO 0
#define DISPLAY_TYPE_WARN 1
#define DISPLAY_TYPE_ERR 2
#define DISPLAY_TYPE_NONE 3

namespace util {
	void displayMessage(std::string msg, int type);

	void displayError(std::string err);
}