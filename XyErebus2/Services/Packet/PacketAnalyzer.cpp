#include "stdafx.h"
#include "..\..\stdafx.h" // intellisense is drunk
#include "..\..\Tools\Detour.h"
#include "PacketAnalyzer.h"
#include <sstream>
#include <iomanip>

LPVOID SendReturnAddress, RecvReturnAddress;

HANDLE pipe = INVALID_HANDLE_VALUE;
void SendToExternalAnalyzer(void* caller, int typeID, int size, BYTE* packet)
{
    try
    {
        if (pipe == INVALID_HANDLE_VALUE)
        {
            pipe = CreateFile("\\\\.\\pipe\\XyErebus2.Bridge", GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
            if (pipe == INVALID_HANDLE_VALUE)
            {
                printf("An error has occured : \n\t@ %s\n\t@ %s\n" , __FILE__, __FUNCSIG__);
                return;
            }
        }

        std::stringstream message;
        message << "packet{";
        message << std::hex << std::setfill('0'); //persisting state

        message << "caller = " << std::setw(8) << caller << ", ";
        message << "type = " << std::setw(4) << typeID << ", ";
        message << "size = " << std::setw(4) << size << ", ";
        message << "data = ";
        for(int i = 0; i < size; i++)
            message << std::setw(2) << int(packet[i]);

        message << "};" << std::endl;

        printf(message.str().c_str());

        DWORD written = 0;
        BOOL result = WriteFile(pipe, message.str().c_str(), message.tellp(), &written, 0);
        if (!result)
        {
            if (GetLastError() == ERROR_NO_DATA)
            {
                pipe = INVALID_HANDLE_VALUE;
                return;
            }
        }
        
        FlushFileBuffers(pipe);
    }
    catch (std::exception &e)
    {
    }
}
void Dispatch_SendPacket(void (*caller), int typeID, int size, void* packet)
{
    //SendToExternalAnalyzer(caller, typeID, size, (BYTE*)packet);

    switch (typeID)
    {
    case 64: //Move to Location
    case 68: //Move a Step
        return;

    case 81:  //not sure what this is, but it get spammed a lot
    case 109: //same
        return;

    case 204: //notify charging spell
    case 61: //appears related to spell finish charging (sometime gets sent twice
        break;

    case 219: //@69D0E6 cast spell (targeted location teleport)
    case 203: //@696584 cast spell (linear aoe)
    case 209: //@697087 cast spell (chain lightning)
              //    * chain lightning sends a 209 per target hits
        break;

    default: break;
    }

    printf("%08X calls Send[%3i] : size%4i\n", caller, typeID, size);
}

void __declspec(naked) Hook_SendPacket()
{
    //00465FF0 - 55                    - push ebp
    //00465FF1 - 8B EC                 - mov ebp,esp
    //00465FF3 - 83 EC 0C              - sub esp,0C
    //00465FF6 - 89 4D F8              - mov [ebp-08],ecx
    //00465FF9 - 81 7D 08 88000000     - cmp [ebp+08],00000088

    __asm
    {
        push ebp
        mov ebp,esp
        sub esp, 0x0C

        pushad
        mov eax, [ebp + 0x10] // packet
        push eax
        mov eax, [ebp + 0x0C] // size
        push eax
        mov eax, [ebp + 0x08] // type id
        push eax
        mov eax, [ebp + 0x04] // type id
        push eax
		call Dispatch_SendPacket
		add esp, 0x10
		popad

        jmp SendReturnAddress
    }
}

void PacketAnalyzer::InstallHook()
{
    printf("Installing Hook\n");
	const int JumpInstructionSize = 1+4;
    const int SendAddress = 0x00465FF0, SendNopCount = 1;
    DWORD protection;

    VirtualProtect((LPVOID)SendAddress, JumpInstructionSize+SendNopCount, PAGE_EXECUTE_READWRITE, &protection);
	Detour::Install(SendAddress, Hook_SendPacket, SendNopCount, true);
    SendReturnAddress = (LPVOID)(SendAddress + JumpInstructionSize + SendNopCount);
}