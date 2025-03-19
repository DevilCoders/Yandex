#pragma once

#include "seinfo.h"

namespace NSe
{

struct TRegexpResult {
    ESearchFlags Flags = SF_NO_FLAG;
    ESearchType Type = ST_UNKNOWN;

    // No flags or types
    enum ENamedMatch {
        ENGINE_BEGIN,
        ENGINE_END,

        QUERY_BEGIN,
        QUERY_END,

        LOW_QUERY_BEGIN,
        LOW_QUERY_END,

        ENCRYPTED_QUERY_BEGIN,
        ENCRYPTED_QUERY_END,

        PLATFORM_BEGIN,
        PLATFORM_END,

        PSTART_BEGIN,
        PSTART_END,

        PSIZE_BEGIN,
        PSIZE_END,

        PNUM_BEGIN,
        PNUM_END,

        TOTAL_MATCH_NUMBER,
    };

    const char* Matches[TOTAL_MATCH_NUMBER] = {nullptr};

    TStringBuf GetStringBetween(ENamedMatch begin, ENamedMatch end) {
        if (!Matches[begin] || !Matches[end])
            return TStringBuf();
        return TStringBuf(Matches[begin], Matches[end]);
    }
};

bool SERecognizeUrl(TRegexpResult& result, const TStringBuf& url);

}; // namespace NSe
