#include "sent.h"

#include <kernel/tarc/iface/tarcface.h>

namespace NSnippets {
    TArchiveSent::TArchiveSent(EARC sourceArc, int sentId, const TUtf16String& rawSent, ui16 sentFlags)
      : SourceArc(sourceArc)
      , SentId(sentId)
      , RawSent(rawSent)
      , Sent(RawSent.data(), RawSent.size())
      , Attr()
      , IsParaStart(sentFlags & SENT_IS_PARABEG)
    {
        if (sentFlags & SENT_HAS_ATTRS) {
            const size_t t = RawSent.find(wchar16('\t'));
            if (t != TUtf16String::npos) {
                RawSent[t] = '\0';
                Sent = TWtringBuf(RawSent.data(), t);
                Attr = TWtringBuf(RawSent.data() + t + 1, RawSent.size() - t - 1);
            }
        }
    }
}
