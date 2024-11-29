#pragma once
#include "pch.h"

namespace Inject {
    void WriteToMemory(DWORD addressToWrite, void* valueToWrite, int byteNum);
}