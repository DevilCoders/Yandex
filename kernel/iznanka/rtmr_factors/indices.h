#pragma once

#include <util/generic/vector.h>
#include <util/system/types.h>

namespace NIznanka {

enum class ERtmrFactorIndex : ui16 {
    // User activity
    UserNothing = 0,
    UserLine = 1,
    UserTeaser = 2,
    UserOpened = 3,
    UserMaximized = 4,
    UserClick10Seconds = 5,
    UserClickMinute = 6,
    UserClick10Minutes = 7,
    UserClickHour = 8,
    UserClickMonth = 9,
    UserReduceToLine = 10,
    UserReduceToTeaser = 11,
    UserReduceToLineDwelltimePrev = 12,
    UserReduceToTeaserDwelltimePrev = 13,

    // User activity on req type
    UserReqTypeNothing = 14,
    UserReqTypeLine = 15,
    UserReqTypeTeaser = 16,
    UserReqTypeOpened = 17,
    UserReqTypeMaximized = 18,
    UserReqTypeClick10Seconds = 19,
    UserReqTypeClickMinute = 20,
    UserReqTypeClick10Minutes = 21,
    UserReqTypeClickHour = 22,
    UserReqTypeClickMonth = 23,
    UserReqTypeReduceToLine = 24,
    UserReqTypeReduceToTeaser = 25,
    UserReqTypeReduceToLineDwelltimePrev = 26,
    UserReqTypeReduceToTeaserDwelltimePrev = 27,

    // User activity on this url
    UserUrlNothing = 28,
    UserUrlLine = 29,
    UserUrlTeaser = 30,
    UserUrlOpened = 31,
    UserUrlMaximized = 32,
    UserUrlClick10Seconds = 33,
    UserUrlClickMinute = 34,
    UserUrlClick10Minutes = 35,
    UserUrlClickHour = 36,
    UserUrlClickMonth = 37,
    UserUrlReduceToLine = 38,
    UserUrlReduceToTeaser = 39,
    UserUrlReduceToLineDwelltimePrev = 40,
    UserUrlReduceToTeaserDwelltimePrev = 41,
    UserUrlReduceToOldness = 42,
};

enum class EFactorType : ui8 {
    User = 0,
    ReqType = 1,
    Url = 2
};

struct TTripleIndex {
    TVector<ERtmrFactorIndex> Indexes;
    TTripleIndex(ERtmrFactorIndex user, ERtmrFactorIndex reqType, ERtmrFactorIndex url);

    ERtmrFactorIndex Get(EFactorType factorType) const;
};

enum class ENavigAction : ui8 {
    ClickBack /* "A9" */,
    ClickOutsideIznanka /* "A18" */,
    SwipeToTeaser /* "A19" */,
    SwipeToLine /* "A20" */,
};

enum class ENavigPath : ui8 {
    Empty /* "" */,
    Nothing /* "2895.292" */,
    Line /* "2895.2355" */,
    Teaser /* "2895.764" */,
    Opened /* "2895.2897" */,
    Maximized /* "2895.2896" */,
};

enum class EFixedReqType : ui8 {
    Unknown /* "" */,
    Web /* "pageload" */,
    Geo /* "geo" */,
    Video /* "videoplay" */,
    Sovetnik /* "sovetnik" */,
};

enum class ERequestType : ui8 {
    Unknown /* "" */,
    Pageload /* "pageload" */,
    Video /* "videoplay" */,
    Sovetnik /* "sovetnik" */,
};

}   // namespace NIznanka
