#pragma once

#include <util/generic/array_ref.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NDwarf {
    struct TLineInfo {
        TString FileName;
        int Line;
        int Col;
        TString FunctionName;
        const void* Address;
    };

    // Resolves backtrace addresses and for each address returns all line infos
    // of inlined functions there.
    TVector<TLineInfo> ResolveBackTrace(TArrayRef<const void* const> backtrace);
}
