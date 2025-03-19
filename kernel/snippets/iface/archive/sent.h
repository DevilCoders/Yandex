#pragma once

#include "enums.h"

#include <util/generic/intrlist.h>
#include <util/generic/list.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/ptr.h>

namespace NSnippets {
    class TArchiveSent : public TIntrusiveListItem<TArchiveSent> {
    public:
        EARC SourceArc;
        int SentId;
        TUtf16String RawSent;
        TWtringBuf Sent;
        TWtringBuf Attr;
        bool IsParaStart;
        TArchiveSent() = default;
        TArchiveSent(EARC sourceArc, int sentId, const TUtf16String& rawSent, ui16 sentFlags);
        void ReplaceSent(const TUtf16String& newSent) {
            Y_ASSERT(newSent.size() == Sent.size());
            RawSent = newSent + Attr;
            Sent = TWtringBuf(RawSent.data(), RawSent.data() + newSent.size());
            Attr = TWtringBuf(RawSent.data() + newSent.size(), RawSent.data() + RawSent.size());
        }
    };
    typedef TIntrusiveListWithAutoDelete<TArchiveSent, TDelete> TArchiveSentList;
}
