#include "mlock.h"

#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/system/mlock.h>

namespace NCloud {

////////////////////////////////////////////////////////////////////////////////

static std::pair<const void*, size_t> ParseMemRange(const TString& line)
{
    char addressStr[64];
    char permsStr[64];
    char offsetStr[64];
    char devStr[64];
    int inode;
    if (sscanf(line.c_str(), "%s %s %s %s %d",
               addressStr,
               permsStr,
               offsetStr,
               devStr,
               &inode) != 5)
    {
        return {};
    }

    if (!inode) {
        return {};
    }

    if (permsStr[0] != 'r') {
        return {};
    }

    uintptr_t startAddress;
    uintptr_t endAddress;
    if (sscanf(
        addressStr, "%" PRIx64 "-%" PRIx64,
        &startAddress,
        &endAddress) != 2)
    {
        return {};
    }

    return {
        reinterpret_cast<const void*>(startAddress),
        static_cast<size_t>(endAddress - startAddress)
    };
}

void LockProcessMemory(TLog& Log)
{
    try {
        TIFStream file("/proc/self/maps");

        TString line;
        while (file.ReadLine(line)) {
            auto [ptr, size] = ParseMemRange(line);
            if (!size) {
                continue;
            }

            try {
                LockMemory(ptr, size);
            } catch (...) {
                STORAGE_WARN(
                    "LockProcessMemory: can't lock memory region ("
                    << size << " bytes), "
                    << CurrentExceptionMessage());
            }
        }
    } catch (...) {
        STORAGE_WARN(
            "LockProcessMemory: " << CurrentExceptionMessage());
    }
}

}   // namespace NCloud
