#include "Application.h"

int main()
{
	try
	{
		Application app;
		app.Run();
		app.Cleanup();
	}
	catch (const std::exception& e)
	{
		spdlog::error("Application error: {}", e.what());
		return -1;
	}
	return 0;
}
