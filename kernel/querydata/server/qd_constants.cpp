#include "qd_constants.h"

#include <util/string/builder.h>

namespace NQueryData {

    TServerOpts TServerOpts::NoLimits() {
        TServerOpts opts;
        opts.MaxTrieRAMSize = -1;
        opts.MaxTotalRAMSize = -1;
        return opts;
    }

    TString TServerOpts::Report() const {
        return TStringBuilder()
            << "MaxTrieRAMSize: " << MaxTrieRAMSize << ", "
            << "MaxTotalRAMSize: " << MaxTotalRAMSize << ", "
            << "MinTrieFastMMapSize: " << MinTrieFastMMapSize << ", "
            << "EnableFastMMap: " << EnableFastMMap << ", "
            << "AlwaysUseMMap: " << AlwaysUseMMap << ", "
            << "EnableDebugInfo: " << EnableDebugInfo << ", "
            << "FailOnDataLoadErrors: " << FailOnDataLoadErrors << ", "
            << "LockInRAM: " << LockInRAM;
    }

}
