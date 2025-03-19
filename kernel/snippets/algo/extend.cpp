#include "extend.h"

#include <kernel/snippets/sent_match/length.h>
#include <kernel/snippets/sent_match/restr.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/snip_builder/snip_builder.h>
#include <util/stream/output.h>

namespace NSnippets {
namespace NSnippetExtend {
    TSnip GetSnippet(const TSnip& mainSnip, const IRestr& restr, const TSentsMatchInfo& sentsMatchInfo, const TWordSpanLen& wordSpanLen, int maxSize) {
        const TSentsInfo& info = sentsMatchInfo.SentsInfo;
        TSnipBuilder b(sentsMatchInfo, wordSpanLen, maxSize, maxSize);
        for (TSnip::TSnips::const_iterator it = mainSnip.Snips.begin(); it != mainSnip.Snips.end(); ++it) {
            (void)b.Add(info.WordId2SentWord(it->GetFirstWord()), info.WordId2SentWord(it->GetLastWord()));
        }
        (void)b.GrowLeftToSent();
        (void)b.GrowRightToSent();
        while (b.GrowRightMoreSent(restr)) {
            continue;
        }
        b.GlueTornSents();
        b.GlueSents(restr);
        //TODO: упячка! сравнивать этот TSnip по весу нельзя
        return b.Get(InvalidWeight);
    }
} }
