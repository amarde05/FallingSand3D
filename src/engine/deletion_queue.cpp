#include "deletionQueue.h"

namespace engine {
	void DeletionQueue::pushFunction(std::function<void()>&& function) {
		deletors.push_back(function);
	}

	void DeletionQueue::flush() {
		// Reverse iterate the queue to execute all functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)(); // Call the function
		}

		deletors.clear();
	}
}