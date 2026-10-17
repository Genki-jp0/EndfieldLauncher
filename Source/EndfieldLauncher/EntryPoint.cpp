#include "stdafx.h"
#include "Launcher.h"

// Arguments are unused; Options::Parse reads the command line itself.
int APIENTRY wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
	return Launcher::Run();
}
