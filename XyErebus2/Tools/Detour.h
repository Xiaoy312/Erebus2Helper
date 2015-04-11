#pragma once
class Detour
{
public:
    // Make a detour at targeted address to a new function
    //      address : address where the call/jmp will be made
    //      detour : a function that will handle the detour.
    //      nopCount : write leading nop(0x90), so the instructions that follow can be parsed nicely
    //      useJump : whether to use jmp or call
    // 
    // NOTE : the function provided must be declared as __declspec(naked), 
    // else the stack protection will fuck up the whole environment for us.
	static void Install(int address, void(*detour), size_t nopCount = 0, bool useJump = false)
	{
		__asm
		{
			mov esi, address
			mov edi, detour
			mov ecx, nopCount
			movzx edx, useJump
			//compute offset
			sub edi, esi
			sub edi, 5

			test edx, edx
			jnz _Jump
			//_Call:
			mov al, 0xE8
			jmp _EndIfJump
_Jump:
			mov al, 0xE9
_EndIfJump:
		    mov [esi], al//Call/Jmp nakedFn
			mov [esi+1], edi

			add esi, 5	//move the esi(addr) by 5bytes 
			mov al, 90h //nop
_Begin:
			//check if still need to nop
			cmp ecx, 0
			je _Exit

			mov [esi], al

			inc esi //move a byte
			dec ecx //decreace the counter
			jmp _Begin
_Exit:
		}
	}
};
