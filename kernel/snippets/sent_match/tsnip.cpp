#include "tsnip.h"
#include "sent_match.h"
#include "glue.h"
#include "internal_links.h"

#include <kernel/snippets/iface/archive/sent.h>
#include <kernel/snippets/config/config.h>

#include <kernel/snippets/factors/factorbuf.h>
#include <kernel/snippets/factors/factors.h>

#include <kernel/snippets/sent_info/sent_info.h>

#include <kernel/snippets/smartcut/consts.h>
#include <kernel/snippets/strhl/hilite_mark.h>

#include <library/cpp/charset/wide.h>

#include <util/generic/hash_set.h>
#include <util/generic/ptr.h>
#include <util/generic/singleton.h>

namespace NSnippets {
    const TInvalidWeight InvalidWeight = TInvalidWeight();

    struct TEllipsisMarks {
        THolder<THiliteMark> Res[2][2];
        TEllipsisMarks()
        {
            for (int i = 0; i < 2; ++i) {
                for (int j = 0; j < 2; ++j) {
                    Res[i][j].Reset(new THiliteMark(i ? BOUNDARY_ELLIPSIS : TUtf16String(), j ? BOUNDARY_ELLIPSIS : TUtf16String()));
                }
            }
        }
        const THiliteMark* GetEllipsisMarks(bool atBegin, bool atEnd) const {
            return Res[atBegin][atEnd].Get();
        }
    };

    static inline TZonedString SnipGetZonedText(
        const TSingleSnip& snip,
        const THiliteMark* es,
        const TVector<std::pair<int, int>>& ext,
        const TVector<TZonedString::TSpan>& linkSpans)
    {
        TGluer g(&snip, es);
        g.MarkMatches(nullptr);
        g.MarkMatchedPhones(nullptr);
        g.MarkAllPhones(nullptr);
        g.MarkPunctTrash(nullptr);
        g.MarkParabeg(nullptr);
        g.MarkExt(ext, nullptr);
        g.MarkFio(nullptr);
        g.MarkSentences(nullptr);
        g.MarkMenuWords(nullptr);
        g.MarkLinks(linkSpans, nullptr);
        g.MarkTableCells(nullptr);
        return g.GlueToZonedString();
    }

    TSnip::TSnip()
    {
    }

    TSnip::TSnip(const TSnips& snips, const TInvalidWeight&)
        : Snips(snips)
    {
    }

    TSnip::TSnip(const TSingleSnip& snip, const TInvalidWeight&)
        : Snips(1, snip)
    {
    }

    TSnip::TSnip(const TSnips& snips, double weight, TFactorStorage factors)
        : Snips(snips)
        , Weight(weight)
        , Factors(std::move(factors))
    {
    }

    TSnip::TSnip(const TSingleSnip& snip, double weight, TFactorStorage factors)
        : Snips(1, snip)
        , Weight(weight)
        , Factors(std::move(factors))
    {
    }

    bool TSnip::HasMatches() const
    {
        for (const TSingleSnip& ssnip : Snips) {
            if (ssnip.HasMatches()) {
                return true;
            }
        }
        return false;
    }

    int TSnip::WordsCount() const
    {
        int res = 0;
        for (const TSingleSnip& ssnip : Snips) {
            res += ssnip.WordsCount();
        }
        return res;
    }

    TUtf16String TSnip::GetRawTextWithEllipsis() const
    {
        const wchar16 SPACE_CHAR = ' ';
        size_t count = Snips.size();
        size_t index = 0;
        TUtf16String res;
        for (const TSingleSnip& ssnip : Snips) {
            const TSentsInfo& si = ssnip.GetSentsMatchInfo()->SentsInfo;
            const int i = ssnip.GetFirstWord();
            const int j = ssnip.GetLastWord();
            res += si.GetTextWithEllipsis(i, j);
            if (index + 1 < count && si.IsWordIdLastInSent(j)) {
                res += SPACE_CHAR;
            }
            ++index;
        }
        return res;
    }

    void TSnip::GuessAttrs() {
        for (TSingleSnip& ssnip : Snips) {
            int firstSent = ssnip.GetFirstSent();
            int lastSent = ssnip.GetLastSent();
            for (int j = firstSent; j <= lastSent; ++j) {
                const TArchiveSent& archiveSent = ssnip.GetSentsMatchInfo()->SentsInfo.GetArchiveSent(j);
                if (archiveSent.Attr) {
                    ssnip.SetPassageAttrs(WideToChar(archiveSent.Attr, CODES_YANDEX));
                    break;
                }
            }
        }
    }

    TVector<TString> TSnip::DumpAttrs() const {
        TVector<TString> attrVec;
        for (const TSingleSnip& ssnip : Snips) {
            attrVec.push_back(ssnip.GetPassageAttrs());
        }
        return attrVec;
    }

    TVector<TZonedString> TSnip::GlueToZonedVec(bool allDots, const TSnip& ext) const
    {
        TVector< std::pair<int, int> > ex;
        for (const TSingleSnip& extSnip : ext.Snips) {
            ex.push_back(extSnip.GetWordRange());
        }
        const size_t snipSize = Snips.size();
        TVector<TZonedString> res;
        res.reserve(snipSize);

        // Collect spans with internal links. To do this We should know some info about all frags during processing
        TVector<TZonedString::TSpan> linkSpans;
        if (!Snips.empty()) {
            if (Snips.begin()->GetSentsMatchInfo()->Cfg.LinksInSnip()) {
                GetLinkSpans(*this, linkSpans);
                Sort(linkSpans.begin(), linkSpans.end(), LsBuf);
            }
        }

        size_t cnt = 0;
        for (const TSingleSnip& ss : Snips) {
            bool dotsAtBegin = (allDots || ss.GetAllowInnerDots() || cnt == 0) && (ss.BeginsWithSentBreak() || ss.GetForceDotsAtBegin());
            bool dotsAtEnd = (allDots || ss.GetAllowInnerDots() || cnt + 1 == snipSize) && (ss.EndsWithSentBreak() || ss.GetForceDotsAtEnd());
            const THiliteMark* es = Singleton<TEllipsisMarks>()->GetEllipsisMarks(dotsAtBegin, dotsAtEnd);
            res.push_back(SnipGetZonedText(ss, es, ex, linkSpans));
            ++cnt;
        }
        return res;
    }

    bool TSnip::ContainsMetaDescr() const
    {
        for (const TSingleSnip& ssnip : Snips) {
            if (ssnip.GetSentsMatchInfo()->SentsInfo.SentVal[ssnip.GetFirstSent()].SourceType == SST_META_DESCR) {
                return true;
            }
        }
        return false;
    }

    bool TSnip::RemoveDuplicateSnips()
    {
        bool removed = false;
        THashSet<size_t> uniqueSnips;
        for (TSnip::TSnips::iterator it = Snips.begin(); it != Snips.end();) {
            if (!uniqueSnips.insert(it->GetWordsHash()).second) {
                it = Snips.erase(it);
                removed = true;
            } else {
                ++it;
            }
        }
        return removed;
    }
}
