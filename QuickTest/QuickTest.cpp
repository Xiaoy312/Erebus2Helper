#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <Windows.h>
#include <stdio.h>
#include <fstream>
#include <tlhelp32.h>

#define MODULE_NAME "XyErebus2.dll"

bool FileExists(const char *path)
{
	std::ifstream ifile(path);
	return ifile;
}

void Inject(HANDLE process, LPCSTR modulePath)
{
	if(!FileExists(modulePath))
	{
		MessageBox(0, modulePath, "File Not Found", 0);
		return;
	}

    LPVOID address = nullptr;
    BOOL success = FALSE;
    HMODULE kernel32 = nullptr;
    HANDLE thread = nullptr;

    __try
    {
        // Inject the dll path into the target process
	    address = VirtualAllocEx(process, NULL, strlen(modulePath), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	    printf("Address allocated : %X\n", address);
        success = WriteProcessMemory(process, (LPVOID)address, modulePath, strlen(modulePath), NULL);
	    printf("WriteProcessMemory : %s\n", success? "successed":"failed");

        // Invoke LoadLibrary via CreateRemoteThread
	    kernel32 = GetModuleHandle("kernel32.dll");
	    printf("Kernel32 : %X\n", kernel32);
	    thread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(kernel32, "LoadLibraryA"), (LPVOID)address, 0, NULL);
	    printf("Remote thread handle : %X\n", thread);
    }
    __finally
    {
	    if(thread) CloseHandle(thread);
	    if(kernel32) FreeLibrary(kernel32);
	    if(address) VirtualFreeEx(process, NULL, (size_t)strlen(modulePath), MEM_RESERVE|MEM_COMMIT);
    }
}

HANDLE FindHostProcess(const char * name)
{
	PROCESSENTRY32 entry; entry.dwSize = sizeof(PROCESSENTRY32);
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            if (_stricmp(entry.szExeFile, name) == 0)
            {  
                return OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
            }
        }
    }
    CloseHandle(snapshot);
}

int main(int argc, char* argv[])
{
    char path[255];
    GetFullPathName(MODULE_NAME, 255, path, 0);
    HANDLE process = FindHostProcess("Erebus2.exe");

	Inject(process, path);
    CloseHandle(process);

	return 0;
}