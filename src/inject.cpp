#include "inject.h"

namespace Inject {
    void WriteToMemory(DWORD addressToWrite, void* valueToWrite, int byteNum)
    {
        //used to change our file access type, stores the old
        //access type and restores it after memory is written
        unsigned long OldProtection;
        //give that address read and write permissions and store the old permissions at oldProtection
        VirtualProtect((LPVOID)(addressToWrite), byteNum, PAGE_EXECUTE_READWRITE, &OldProtection);

        //write the memory into the program and overwrite previous value
        memcpy((LPVOID)addressToWrite, valueToWrite, byteNum);

        //reset the permissions of the address back to oldProtection after writting memory
        VirtualProtect((LPVOID)(addressToWrite), byteNum, OldProtection, &OldProtection);
    }
}