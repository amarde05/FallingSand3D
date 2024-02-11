#pragma once

#include <functional>
#include <deque>

namespace engine {
	namespace rendering {
		struct DeletionQueue {
			std::deque<std::function<void()>> deletors;

			void pushFunction(std::function<void()>&& function);

			void flush();
		};
	}
}