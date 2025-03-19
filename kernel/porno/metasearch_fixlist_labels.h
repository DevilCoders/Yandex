#pragma once

#include <library/cpp/enumbitset/enumbitset.h>

enum EMetaSearchFixlistLabel {
    MSFL_EXPLICIT = 0 /* "explicit" */,
    MSFL_PERVERSION /* "perversion" */,
    MSFL_GRUESOME /* "gruesome" */,
    MSFL_ILLEGAL /* "illegal" */,
    MSFL_MAX
};

using TMetaSearchFixlistLabelsBitMask = TEnumBitSet<EMetaSearchFixlistLabel, MSFL_EXPLICIT, MSFL_MAX>;

TString GetMSFLBitMaskReadableRepr(const TMetaSearchFixlistLabelsBitMask& mask);

bool FromString(const TStringBuf& name, EMetaSearchFixlistLabel& ret);
