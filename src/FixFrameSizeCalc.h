#pragma once

#include "CommonHeaders.h"

#pragma pack(push, 1)
// RUNTIME_FUNCTION in 64-bit PE files
struct RUNTIME_FUNCTION {
    uint32 BeginAddress;
    uint32 EndAddress;
    uint32 UnwindData;
};

// UNWIND_INFO Header
struct UNWIND_INFO_HEADER
{
    uint8 Version : 3;
    uint8 Flags : 5;
    uint8 PrologSize;
    uint8 CountOfCodes;
    uint8 FrameRegister : 4;
    uint8 FrameOffset : 4;
};

// UNWIND_CODE
struct UNWIND_CODE
{
    uint8 CodeOffset;
    uint8 UnwindOp : 4;
    uint8 OpInfo : 4;
};
#pragma pack(pop)

class FixFrameSizeCalc {
public:
    FixFrameSizeCalc();
    ~FixFrameSizeCalc();
};