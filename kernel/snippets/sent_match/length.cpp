#include "length.h"

#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/single_snip.h>

#include <kernel/snippets/sent_info/sent_info.h>

#include <kernel/snippets/smartcut/cutparam.h>
#include <kernel/snippets/smartcut/snip_length.h>
#include <kernel/snippets/smartcut/wordspan_length.h>

namespace NSnippets
{
    static TTextFragment ToTextFragment(int firstWord, int lastWord, const TSentsInfo& sentsInfo) {
        TTextFragment fragment;
        fragment.TextBeginOfs = sentsInfo.WordVal[firstWord].TextBufBegin;
        fragment.PrependEllipsis = !sentsInfo.IsWordIdFirstInSent(firstWord);
        fragment.TextEndOfs = sentsInfo.WordVal[lastWord].TextBufEnd;
        fragment.AppendEllipsis = !sentsInfo.IsWordIdLastInSent(lastWord);
        return fragment;
    }

    static TTextFragment ToTextFragment(const TSingleSnip& ssnip, bool isPixel) {
        TTextFragment fragment = ToTextFragment(ssnip.GetFirstWord(), ssnip.GetLastWord(), ssnip.GetSentsMatchInfo()->SentsInfo);
        if (isPixel) {
            fragment.Calculator = &ssnip.GetSentsMatchInfo()->GetPixelLengthCalculator();
        }
        return fragment;
    }

    TWordSpanLen::TWordSpanLen(const TCutParams& cutParams)
        : IsPixel(cutParams.IsPixel)
        , Calc(new TSnipLengthCalculator(cutParams))
    {
    }

    TWordSpanLen::~TWordSpanLen() {
    }

    float TWordSpanLen::CalcLength(const TSingleSnip& fragment) const {
        return Calc->CalcLengthSingle(ToTextFragment(fragment, IsPixel));
    }

    float TWordSpanLen::CalcLength(const TVector<TSingleSnip>& fragments) const {
        TVector<TTextFragment> textFragments;
        for (const TSingleSnip& fragment : fragments) {
            textFragments.push_back(ToTextFragment(fragment, IsPixel));
        }
        return Calc->CalcLengthMulti(textFragments);
    }

    float TWordSpanLen::CalcLength(const TList<TSingleSnip>& fragments) const {
        return CalcLength(TVector<TSingleSnip>(fragments.begin(), fragments.end()));
    }

    int TWordSpanLen::FindFirstWordLonger(const TSentsMatchInfo& matchInfo, int firstWord, int lastWord, float maxLength) const {
        TMakeFragment makeFragment = [&](int aFirstWord, int aLastWord) {
            TTextFragment fragment = ToTextFragment(aFirstWord, aLastWord, matchInfo.SentsInfo);
            if (IsPixel) {
                fragment.Calculator = &matchInfo.GetPixelLengthCalculator();
            }
            return fragment;
        };
        TWordSpanLengthCalculator wordCalc(makeFragment, *Calc);
        return wordCalc.FindFirstWordLonger(firstWord, lastWord, maxLength);
    }
}
