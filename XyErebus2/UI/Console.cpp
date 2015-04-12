#include "stdafx.h"
#include "Console.h"
#include <stdio.h>

LRESULT CALLBACK ConsoleProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);
WNDPROC originalProc;

void Console::Initialize()
{
#if !defined(_DEBUG)
    return;
#endif


    if (!AllocConsole())
    {
            MessageBox(0, "Failed to alloc console", "Console Initialization Error", 0);
        return;
    }
    
	// Redirect printf and scanf to console
	FILE* f;
	freopen_s(&f, "CONOUT$", "w+t", stdout);
	freopen_s(&f, "CONIN$", "w+t", stdin);

	// Redirect windows procedure
	originalProc = (WNDPROC)SetWindowLong(GetConsoleWindow(), GWL_WNDPROC, (LONG)ConsoleProc);

    SetConsoleTitle("XyErebus2 Debug Console");
	printf("Last compiled @ %s %s\n", __DATE__, __TIME__);
}

LRESULT CALLBACK ConsoleProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    // Prevent the console from being closed which in turn kills the host application
	if (Message == WM_CLOSE)
		return ShowWindow(hWnd,HIDE_WINDOW);

	return originalProc(hWnd,Message,wParam,lParam);
}