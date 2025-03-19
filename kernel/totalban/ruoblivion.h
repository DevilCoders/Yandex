#pragma once

#include <kernel/qtree/richrequest/richnode_fwd.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NTotalBan {
    struct TRuOblivionResult {
        TString QueryMatch;
        bool FlagRTBF = false;

        TRuOblivionResult() = default;

        TRuOblivionResult(const TString& match, bool flagRTBF)
            : QueryMatch(match)
            , FlagRTBF(flagRTBF)
        {}

        explicit operator bool() const {
            return !!QueryMatch;
        }
    };

    TString MergeRuOblivionParameters(const TVector<TStringBuf>& params);

    TRuOblivionResult FindRuOblivionMatch(TStringBuf params, TRichTreeConstPtr);
}
