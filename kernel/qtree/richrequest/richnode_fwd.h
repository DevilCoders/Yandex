#pragma once

#include <util/generic/fwd.h>

class TRichRequestNode;

namespace NSearchQuery {
    class TRequest;
}

class TBlob;

using TRichNodePtr = TIntrusivePtr<TRichRequestNode>;
using TBinaryRichTree = TBlob;
using TConstNodesVector = TVector<const TRichRequestNode*>;
using TRichTreePtr=TIntrusivePtr<NSearchQuery::TRequest>;
using TRichTreeConstPtr=TIntrusiveConstPtr<NSearchQuery::TRequest>;

/// Flags for the printer
enum EPrintRichRequestOptions {
    PRRO_Default = 0,
    //legacy flag was deleted: PRRO_Reqs = 1,
    PRRO_WordNoMiscOps = 2,
    PRRO_ProxInfo = 4,
    PRRO_StopWords = 8,
    PRRO_RevFreqs = 16,
    PRRO_IgnorePunct = 32,
    // do not print miscops, markup and node necessities
    PRRO_IgnoreMetaData = 64
};
