#pragma once
#include "pch.h"

namespace Inject {
    void MakeJMP(BYTE* pAddress, DWORD dwJumpTo, DWORD dwLen);
    void Nop(BYTE* pAddress, DWORD dwLen);
    void WriteToMemory(DWORD addressToWrite, void* valueToWrite, int byteNum);
}