#pragma once

#include <util/datetime/base.h>
#include <util/generic/string.h>

namespace NSnippets {

    struct TRangeTime {
        TDuration From = TDuration::Zero();
        TDuration To = TDuration::Max();

        bool Fits(const TDuration& t) const;
        void Parse(const TString& s);
    };

} //namespace NSnippets
