#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <cstdlib>

#include "Application.h"

int main()
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
#endif

	{
		Application app;
		app.Run();
		app.Cleanup();
	}

	return 0;
}
