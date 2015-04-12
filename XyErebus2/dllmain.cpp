// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "UI/Console.h"
#include "Services\Packet\PacketAnalyzer.h"
#include "Services\Hack\MapNuke.h"
#include "Services\Hack\Patch.h"
#include "Services\Hack\InventoryHelper.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	    Console::Initialize();
        PacketAnalyzer::InstallHook();
        MapNuke::Initialize();
        Patch::Initialize();
        InventoryHelper::Initialize();

        break;
    case DLL_PROCESS_DETACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

