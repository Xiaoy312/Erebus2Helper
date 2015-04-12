#include "stdafx.h"
#include "../ServiceBase.h"
#include "MapNuke.h"
#include "../../Tools/Detour.h"

LPVOID ReturnAddress;

void ChangeRadius(float* radius, float* distance)
{
    // the function check for mob within a rect of size (radius x (radius+distance))
    // apparently the server also do a final distance check to prevent hacking;
    // however, if final distance is less or equal to radius plus distance, it is still valid
    // using this on linear aoe skill, we can make it a self-centered aoe.
    /*   +-radius-+
         |        |
         |        |distance
         |        |
         |        |
         +-radius-+
         |    .   |
         |        |
         +-radius-+
    */
    *radius = *radius + *distance;
    *distance = 0.0;
}
void __declspec(naked) Hook_GetValidTargetVector()
{
    //00694600 - 6A FF                 - push FF
    //00694602 - 68 E6237300           - push 007323E6 : [0824548B]
    __asm
    {
        pushad //esp + 0x20
        lea eax, [esp + 0x20 + 0x10]
        push eax
        lea eax, [esp + 0x20 + 4 + 0xC]
        push eax
        call ChangeRadius
        add esp, 8
        popad

        push 0xFF
        push 0x007323E6
        jmp ReturnAddress
    }
}

void MapNuke::Initialize()
{
    //00694600 - 6A FF                 - push FF
    //00694602 - 68 E6237300           - push 007323E6 : [0824548B]
	const int JumpInstructionSize = 1+4;
    const int Address = 0x00694600, NopCount = 2;
    DWORD protection;

    VirtualProtect((LPVOID)Address, JumpInstructionSize+NopCount, PAGE_EXECUTE_READWRITE, &protection);
	Detour::Install(Address, Hook_GetValidTargetVector, NopCount, true);
    ReturnAddress = (LPVOID)(Address + JumpInstructionSize + NopCount);
}