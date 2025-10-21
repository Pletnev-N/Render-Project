#include <windows.h>
#include <cstdio>

#include "dxapp.h"



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	DXApp theApp(hInstance);
	if (!theApp.Init())
	{
		printf("init fail");
		return 0;
	}

	printf("init success");
	return theApp.Run();
}
