#include "engine/engine.h"
#include "util/debug.h"

int main(int argc, char* argv[]) {
	engine::VulkanEngine engine("Voxel Game");

	engine.init();

	engine.run();

	engine.cleanup();

	return 0;
}