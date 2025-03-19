#pragma once

#include <util/generic/string.h>
#include <util/system/defaults.h>

namespace NQueryData {

    const ui64 UNLIMITED_MEMORY = -1;

    const ui64 MAX_TRIE_RAM_SIZE = 6ULL << 30; // 6Gb
    const ui64 MIN_TRIE_FAST_MMAP_SIZE = 1ULL << 30; // 1Gb

    const ui64 MAX_MEMORY_USAGE = 21ULL << 29; // 10.5Gb

    enum EStatsVerbosity {
        SV_NONE = 0, SV_MAIN, SV_SOURCES, SV_FACTORS, SV_COUNTERS
    };

    enum EDataLoadErrorsMode {
        DLM_IGNORE, DLM_FAIL
    };

    struct TServerOpts {
        ui64 MaxTrieRAMSize = MAX_TRIE_RAM_SIZE;
        ui64 MaxTotalRAMSize = MAX_MEMORY_USAGE;
        ui64 MinTrieFastMMapSize = MIN_TRIE_FAST_MMAP_SIZE;
        bool EnableFastMMap = false;
        bool AlwaysUseMMap = false;
        bool EnableDebugInfo = false;
        bool FailOnDataLoadErrors = false;
        bool LockInRAM = false;

    public:
        static TServerOpts NoLimits();

        TString Report() const;
    };

}
