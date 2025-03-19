#pragma once

#include <util/generic/vector.h>

namespace NSnippets {
    class TSentsMatchInfo;
    class TSnipTitle;

    class TTitleMatchInfo {
    private:
        struct TWordVal {
            bool IsSuffixOrStopword = false;
        };
        TVector<TWordVal> WordVal;
        struct TSumWord {
            int SumTitleMatches = 0;
            int SumDiscountedTitleMatches = 0;
        };
        TVector<TSumWord> WordAcc;
        struct TSentVal {
            double TitleSimilarity = 0.0;
        };
        TVector<TSentVal> SentVal;

    private:
        int GetRawTitleWordCover(int i, int j) const;

    public:
        void Fill(const TSentsMatchInfo& text, const TSnipTitle* title);
        double GetTitleLikeness(int i, int j) const;
        double GetTitleSimilarity(int i) const;
    };
}
