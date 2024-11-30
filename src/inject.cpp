#include "inject.h"

namespace Inject {
    void Nop(BYTE* pAddress, DWORD dwLen)
    {
        DWORD dwOldProtect, dwBkup;

        // give the paged memory read/write permissions

        VirtualProtect(pAddress, dwLen, PAGE_EXECUTE_READWRITE, &dwOldProtect);

        for (DWORD x = 0x0; x < dwLen; x++) *(pAddress + x) = 0x90;

        // restore the paged memory permissions saved in dwOldProtect

        VirtualProtect(pAddress, dwLen, dwOldProtect, &dwBkup);

        return;

    }

    void MakeJMP(BYTE* pAddress, DWORD dwJumpTo, DWORD dwLen)
    {
        DWORD dwOldProtect, dwBkup, dwRelAddr;

        // give the paged memory read/write permissions

        VirtualProtect(pAddress, dwLen, PAGE_EXECUTE_READWRITE, &dwOldProtect);

        // calculate the distance between our address and our target location
        // and subtract the 5bytes, which is the size of the jmp
        // (0xE9 0xAA 0xBB 0xCC 0xDD) = 5 bytes

        dwRelAddr = (DWORD)(dwJumpTo - (DWORD)pAddress) - 5;

        // overwrite the byte at pAddress with the jmp opcode (0xE9)

        *pAddress = 0xE9;

        // overwrite the next 4 bytes (which is the size of a DWORD)
        // with the dwRelAddr

        *((DWORD*)(pAddress + 0x1)) = dwRelAddr;

        // overwrite the remaining bytes with the NOP opcode (0x90)
        // NOP opcode = No OPeration

        for (DWORD x = 0x5; x < dwLen; x++) *(pAddress + x) = 0x90;

        // restore the paged memory permissions saved in dwOldProtect

        VirtualProtect(pAddress, dwLen, dwOldProtect, &dwBkup);

        return;

    }

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