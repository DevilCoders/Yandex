#pragma once

#include <util/generic/hash.h>
#include <util/generic/vector.h>

namespace NGroupingAttrs {

    TVector<TString> ReadCategToName(const TString& filename);
    THashMap<TString, ui32> ReadNameToCateg(const TString& filename);

    TVector<TString> ReadCategToNameWad(const TString& filename);
    THashMap<TString, ui32> ReadNameToCategWad(const TString& filename);

} // namespace NGroupingAttrs
