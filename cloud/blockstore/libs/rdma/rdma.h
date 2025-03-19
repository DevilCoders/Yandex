#pragma once

#include "public.h"

#include <util/system/types.h>

namespace NCloud::NBlockStore::NRdma {

////////////////////////////////////////////////////////////////////////////////

typedef int TRegisterMemory(void* addr, size_t len, void* cookie);
typedef int TUnregisterMemory(void* addr, size_t len, void* cookie);

struct TRdmaHandler
{
    TRegisterMemory* RegisterMemory = nullptr;
    TUnregisterMemory* UnregisterMemory = nullptr;
};

////////////////////////////////////////////////////////////////////////////////

inline TRdmaHandler RdmaHandlerStub()
{
    return TRdmaHandler();
}

}   // namespace NCloud::NBlockStore::NRdma
