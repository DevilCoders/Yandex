#pragma once

#include <util/generic/string.h>
#include <util/system/defaults.h>
#include <util/stream/output.h>

namespace NReMorph {

struct TRemorphVersion {
    ui16 Major;
    ui16 Minor;
    ui16 Patch;
};

struct TRemorphInfo {
    TRemorphVersion Version;
};

extern const TRemorphInfo REMORPH_INFO;

} // NReMorph

TString ToString(const NReMorph::TRemorphVersion& version);

Y_DECLARE_OUT_SPEC(inline, NReMorph::TRemorphVersion, out, version) {
    out << ::ToString(version);
}

Y_DECLARE_OUT_SPEC(inline, NReMorph::TRemorphInfo, out, info) {
    out << "Remorph " << info.Version << Endl;
}
