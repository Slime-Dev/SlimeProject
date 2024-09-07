#include "SlimeWindow.h"
#include <spdlog/spdlog.h>
#include <vector>
#include <stdexcept>
#include <iostream>


int main() {
	SlimeWindow window = SlimeWindow({ .title = "Slime Odyssey", .width = 1920, .height = 1080, .resizable = true, .decorated = true, .fullscreen = false });

	if (window.GetHeight() != 1080)
	{
		spdlog::error("Failed to set height correctly");
		return 1;
	}
	else if (window.GetWidth() != 1920)
	{
		spdlog::error("Failed to set width correctly");
		return 1;
	}
	else
	{
		spdlog::info("Created window successfully");
	}
}
