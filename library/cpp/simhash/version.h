#pragma once

#include <util/system/defaults.h>

struct TSimHashVersion {
    ui32 NGrammVersion : 5;
    ui32 RandomVersion : 12;
    ui32 VectorizerVersion : 10;
    ui32 MethodVersion : 5;

    TSimHashVersion()
        : NGrammVersion(1)
        , RandomVersion(0)
        , VectorizerVersion(1)
        , MethodVersion(1)
    {
        static_assert((sizeof(ui32) == sizeof(TSimHashVersion)), "expect (sizeof(ui32) == sizeof(TSimHashVersion))");
    }
};
