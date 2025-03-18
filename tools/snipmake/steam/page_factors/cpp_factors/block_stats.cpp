#include "block_stats.h"

#include <library/cpp/tokenizer/split.h>

#include <util/charset/unidata.h>
#include <util/charset/wide.h>
#include <util/string/strip.h>
#include <util/string/vector.h>


namespace NSegmentator {

// TTextStats
TTextStats::TTextStats(const TUtf16String& text)
    : Text(StripString(text))
{}

void TTextStats::CalcTextStats() {
    for (TChar c : Text) {
        if (IsAlpha(c)) {
            ++AlphasCount;
            if (IsUpper(c)) {
                ++UpperAlphasCount;
            }
        } else if (IsDigit(c)) {
            ++DigitsCount;
        } else if (IsPunct(c) || IsSingleQuotation(c)) {
            ++PunctsCount;
            if (c == '.' || c == '!' || c == '?') {
                ++EndPunctsCount;
            } else {
                ++MidPunctsCount;
            }
        }
    }

    WordsCount = SplitIntoTokens(Text).size();
}

void TTextStats::Update(const TTextStats& stats) {
    AlphasCount += stats.AlphasCount;
    UpperAlphasCount += stats.UpperAlphasCount;
    DigitsCount += stats.DigitsCount;
    PunctsCount += stats.PunctsCount;
    MidPunctsCount += stats.MidPunctsCount;
    EndPunctsCount += stats.EndPunctsCount;

    WordsCount += stats.WordsCount;
}


// TBlockStats
TBlockStats::TBlockStats(const TWtringBuf& block)
    : BlockStats(TTextStats(CollapseText(block)))
{
    CalcStats();
}

TUtf16String TBlockStats::CollapseText(const TWtringBuf& text) {
    TUtf16String collapsedText(text);
    Collapse(collapsedText);
    return collapsedText;
}

void TBlockStats::CalcStats() {
    // TODO: split into paragraphs
    ParsStats.push_back(TTextStats(BlockStats.Text));

    SentsInParsCounts.reserve(ParsStats.size());
    for (TTextStats& parStats : ParsStats) {
        TVector<TUtf16String> parSents = SplitIntoSentences(parStats.Text);
        SentsInParsCounts.push_back(parSents.size());

        SentsStats.reserve(SentsStats.size() + parSents.size());
        for (TUtf16String& sent : parSents) {
            SentsStats.push_back(TTextStats(sent));
            SentsStats.back().CalcTextStats();
            parStats.Update(SentsStats.back());
        }
        BlockStats.Update(parStats);
    }

    if (SentsStats.empty()) {
        // put fake empty sentence for calculations
        SentsStats.push_back(TTextStats(TUtf16String()));
    }
}

}  // NSegmentator
