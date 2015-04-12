#include "stdafx.h"
#include "Services\ServiceBase.h"
#include "InventoryHelper.h"
#include "../../Tools/Detour.h"

//0047D71B - 8B 08  - mov ecx,[eax] // pick up item from inventory
//0047B813 - 8B 08  - mov ecx,[eax] // release picked item (to inventory)

// Various type of item have different inventory size, for instance,
// a ring is 1 by 1, a 2handed staff is 1 by 4, a topwear is 2 by 3, etc.
// By altering the size of the item, we can stack an unlimited number of 
// items on the same inventory slot.

/*  how we got there :
    [Part A]
    * we presume that the inventory item structure stores its location in the inventory, and its size.
    - isolate an item's x address, by repeatly moving it horizontally and searching for its x value(either 0 or 1 based)
    - find the address that modify its x, we can find item's base address and x's offset
    - look around the item structure, we can easily locate its size
    [Part B]
    - isolate the address that stores the item that is held in 'mouse'; quickiest way is to search item base when holding it
      and isolate the right one by release it.
    - isolate the functions that write to that address to locate the part that handles the grab and the release of an item
    - hijack these functions to alter on the size of the item : 
        - after picking it up, set the width to 0
        - after releasing, restore the width so it can picked up again
*/

struct DetourLocation
{
    void *DetourAddress;
    void *ReturnAddress;
    size_t NopCount;

    DetourLocation(int address, size_t size) : DetourAddress((void*)address), ReturnAddress((void*)(address + size))
    {
        const int DetourSize =  1 + 4;

        if (size < DetourSize) throw;
        NopCount = size - DetourSize;

        // make the region writable
        DWORD protection;
        VirtualProtect(DetourAddress, size, PAGE_EXECUTE_READWRITE, &protection);
    }
};

#define MACRO_CONCAT_IMPL(x,y) x##y
#define MACRO_CONCAT(x,y) MACRO_CONCAT_IMPL(x,y)
#define PADDING(size) BYTE MACRO_CONCAT(_padding, __COUNTER__)[size]

enum Storage : BYTE
{
    Inventory = 0,
    Bank = 2,
    PetInventory = 17
};
struct InventoryItem
{
    DWORD Handle;               // 0x000..0x003
    PADDING(0x10);              // 0x004..0x013
    Storage Location;           // 0x014
    PADDING(0x1);               // 0x015
    BYTE InventoryTab;          // 0x016
    BYTE X;                     // 0x017
    BYTE Y;                     // 0x018
    PADDING(0xE3);              // 0x019..0x0FB
    BYTE VerticalSize;          // 0x0FC
    BYTE HorizontalSize;        // 0x0FD
};

InventoryItem **GrabbedItem = (InventoryItem**)0x00D26BD0;
BYTE GrabbedItemHorizontalSize;

DetourLocation InventoryPickUp(0x0047D857, 2+4);
DetourLocation Swapped(0x0047C3CB, 2+4);
DetourLocation Bagged(0x0047CDE3, 2+4+4);
DetourLocation Equipped(0x0047C57D, 2+4+4);
DetourLocation Dropped(0x0047EF67, 2+4+4);
DetourLocation BankPickUp(0x00638F76, 1+4);
DetourLocation Banked(0x00638E3C, 2+4+4);

//0047D857 - 89 0D D06BD200  - mov [00D26BD0],ecx                   // pick up from inventory, equip slot, or pet inventory
//0047C3CB - 89 0D D06BD200  - mov [00D26BD0],ecx                   // swap grabbed with equipped
//0047CDE3 - C7 05 D06BD200 00000000 - mov [00D26BD0],00000000      // release : inv, pet inv
//0047C57D - C7 05 D06BD200 00000000 - mov [00D26BD0],00000000      // release : equipped
//0047EF67 - C7 05 D06BD200 00000000 - mov [00D26BD0],00000000      // release : dropped


//00638F76 - A3 D06BD200 - mov [00D26BD0],eax                       // pick up from bank
//00638E3C - C7 05 D06BD200 00000000 - mov [00D26BD0],00000000      // release : bank

void HandlePickUpItem(InventoryItem* item)
{
    // shouldn't happen, but in just case we mess up, restore the item width so it could be picked up again
    if (*GrabbedItem && (*GrabbedItem)->HorizontalSize == 0)
        (*GrabbedItem)->HorizontalSize = max(1, GrabbedItemHorizontalSize);

    *GrabbedItem = item;
    GrabbedItemHorizontalSize = max(1, (*GrabbedItem)->HorizontalSize);
    (*GrabbedItem)->HorizontalSize = 0;
}
void HandleReleaseItem()
{
    if (*GrabbedItem)
    {
        (*GrabbedItem)->HorizontalSize = max(1, GrabbedItemHorizontalSize);

        *GrabbedItem = nullptr;
        GrabbedItemHorizontalSize = 0;
    }
}
void HandleSwapItem(InventoryItem* item)
{
    HandleReleaseItem();
    HandlePickUpItem(item);
}

void __declspec(naked) Detour_InventoryPickUp()
{
    //0047D857 - 89 0D D06BD200  - mov [00D26BD0],ecx                   // pick up from inventory or equip slot
    __asm
    {
        push ecx
        call HandlePickUpItem
        jmp InventoryPickUp.ReturnAddress
    }
}
void __declspec(naked) Detour_Swapped()
{
    //0047C3CB - 89 0D D06BD200  - mov [00D26BD0],ecx                   // swap grabbed with equipped
    __asm
    {
        push ecx
        call HandleSwapItem
        jmp Swapped.ReturnAddress
    }
}
void __declspec(naked) Detour_Bagged()
{
    //0047CDE3 - C7 05 D06BD200 00000000 - mov [00D26BD0],00000000      // release : inv
    __asm
    {
        call HandleReleaseItem
        jmp Bagged.ReturnAddress
    }
}
void __declspec(naked) Detour_Equipped()
{
    //0047C57D - C7 05 D06BD200 00000000 - mov [00D26BD0],00000000      // release : equipped
    __asm
    {
        call HandleReleaseItem
        jmp Equipped.ReturnAddress
    }
}
void __declspec(naked) Detour_Dropped()
{
    //0047EF67 - C7 05 D06BD200 00000000 - mov [00D26BD0],00000000      // release : dropped
    __asm
    {
        call HandleReleaseItem
        jmp Dropped.ReturnAddress
    }
}
void __declspec(naked) Detour_BankPickUp()
{
    //00638F76 - A3 D06BD200 - mov [00D26BD0],eax                       // pick up from bank
    __asm
    {
        push ecx
        call HandlePickUpItem
        jmp BankPickUp.ReturnAddress
    }
}
void __declspec(naked) Detour_Banked()
{
    //00638E3C - C7 05 D06BD200 00000000 - mov [00D26BD0],00000000      // release : bank
    __asm
    {
        call HandleReleaseItem
        jmp Banked.ReturnAddress
    }
}

void InventoryHelper::Initialize()
{
    Detour::Install((int)InventoryPickUp.DetourAddress, Detour_InventoryPickUp, InventoryPickUp.NopCount, true);
    Detour::Install((int)Swapped.DetourAddress, Detour_Swapped, Swapped.NopCount, true);
    Detour::Install((int)Bagged.DetourAddress, Detour_Bagged, Bagged.NopCount, true);
    Detour::Install((int)Equipped.DetourAddress, Detour_Equipped, Equipped.NopCount, true);
    Detour::Install((int)Dropped.DetourAddress, Detour_Dropped, Dropped.NopCount, true);
    Detour::Install((int)BankPickUp.DetourAddress, Detour_BankPickUp, BankPickUp.NopCount, true);
    Detour::Install((int)Banked.DetourAddress, Detour_Banked, Banked.NopCount, true);
}