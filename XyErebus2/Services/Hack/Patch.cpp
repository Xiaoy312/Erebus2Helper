#include "stdafx.h"
#include "Patch.h"

template <size_t length> //Default buffer size
class HotPatch
{
protected:
	void* addr;
	char oldAOB[length];
	char newAOE[length];
	size_t oldSize, newSize;

public:
	template<size_t _1, size_t _2>
	HotPatch(const char (&old)[_1],
			 const char (&new_)[_2],
			 DWORD address)
	{
		oldSize = _1 - 1; //null-terminated
		newSize = _2 - 1;
		if (length < oldSize || length < newSize || oldSize < newSize)
		{
            printf("%i, %i, %i\n", length, oldSize, newSize);
#ifdef _DEBUG
			throw;
#else
			return;
#endif
		}
		memcpy(oldAOB, old, oldSize);
		memcpy(newAOE, new_, newSize);
		addr = (void*)address;

        DWORD protection;
        VirtualProtect((LPVOID)address, length, PAGE_EXECUTE_READWRITE, &protection);
	}

	void Enable(bool enable = true)
	{
		if(enable)
		{
			memcpy(addr, newAOE, newSize);
			if(newSize < oldSize)
				memset((void*)((DWORD)addr + newSize), 0x90, oldSize-newSize);
		}
		else Disable();
	}
	void Disable()
	{
		memcpy(addr, oldAOB, oldSize);
	}

	void Debug()
	{
		printvar("%i",oldSize);
		printvar("%i",newSize);
	}
};

void AlwaysMaxAttackPower(bool enabled)
{
    //006961F1 - E8 BAD2FFFF           - call 006934B0 ; Skill::GetAttackPower();

    /* 
    006934B0 - 83 EC 08              - sub esp,08
    changed to
    006934B0 - B0 64                 - mov al,64
    006934B2 - C3                    - ret 
    */
    HotPatch<3>("\x83\xEC\x08", "\xB0\x64\xC3", 0x006934B0).Enable(enabled);
}
void UnlimitedZoom(bool enabled)
{
    // global variable float 0xD25CE4 : camera distance 
    //004155E8 - D9 98 DC000000  - fstp dword ptr [eax+000000DC] //scrolling
    //0041565A - D9 99 DC000000  - fstp dword ptr [ecx+000000DC] //capped at 2500.0f 
    //004157D2 - D9 99 DC000000  - fstp dword ptr [ecx+000000DC] //capped at 2500.0f
    //0044D9B1 - D9 99 DC000000  - fstp dword ptr [ecx+000000DC] //capped at 2500.0f
    
    //just nop all of them
    HotPatch<6>("\xD9\x99\xDC\x00\x00\x00", "", 0x0041565A).Enable(enabled);
    HotPatch<6>("\xD9\x99\xDC\x00\x00\x00", "", 0x004157D2).Enable(enabled);
    HotPatch<6>("\xD9\x99\xDC\x00\x00\x00", "", 0x0044D9B1).Enable(enabled);
}

void Patch::Initialize()
{
    AlwaysMaxAttackPower(true);
    UnlimitedZoom(true);
}