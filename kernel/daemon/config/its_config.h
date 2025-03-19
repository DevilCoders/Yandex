#pragma once

#include <util/generic/fwd.h>

void HandleItsConfig(
        const TVector<TString>& itsConfigPaths,
        const TString& dstFolder,
        THashMap<TString, TString>& patches
) noexcept;
