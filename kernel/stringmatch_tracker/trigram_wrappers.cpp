#include "trigram_wrappers.h"

namespace NStringMatchTracker {

    ///============TrigramBaseCommon===============

    class TTrigramCalcerCommon {
    private:
        ::NSequences::TTrigramMatcher& Matcher;

    public:
        TTrigramCalcerCommon(::NSequences::TTrigramMatcher& matcher)
            : Matcher(matcher)
        {
        }

        void UpdateBestMatch(const char* addStrBeg, const char* addStrEnd, NPrivate::TCalcerState& state) {
            Y_ASSERT(addStrEnd - addStrBeg >= 0);

            size_t lastMatchPos = addStrEnd - addStrBeg + 10;
            size_t matchCount = 0;

            Matcher.ResetMatch();

            for (auto it = addStrEnd; it != addStrBeg; --it) {
                Matcher.ExtendMatch(*(it - 1));
                if (!Matcher.FindMatch()) {
                    continue;
                }

                matchCount += Min((size_t)3, size_t(lastMatchPos - (it - addStrBeg) + 1));
                lastMatchPos = (it - addStrBeg) - 1;

            }

            if (matchCount == state.BestMatch) {
                state.BestDocLen = Min(state.BestDocLen, size_t(addStrEnd - addStrBeg));
            } else if (matchCount > state.BestMatch) {
                state.BestMatch = matchCount;
                state.BestDocLen = size_t(addStrEnd - addStrBeg);
            }
        }
    };

    ///============TQueryTrigramOverDocCalcer===============
    ECalcer TQueryTrigramOverDocCalcer::GetCalcerType() const {
        return CalcerType;
    }

    void TQueryTrigramOverDocCalcer::NewQuery(const TString& query) {
        TCalcer::NewQuery(query);
        Matcher.Reset(new::NSequences::TTrigramMatcher(query, SplitByWords));
        State.NewQuery(query.size());
        Matcher->ResetMatch();
    }

    void TQueryTrigramOverDocCalcer::ProcessDoc(const char* docBeg, const char* docEnd) {
        Y_ASSERT(Matcher);
        Y_ASSERT(Inited);
        TTrigramCalcerCommon(*Matcher.Get()).UpdateBestMatch(docBeg, docEnd, State);
    }

    void TQueryTrigramOverDocCalcer::Reset() {
        if (Inited) {
            State.ResetMatch();
            Matcher->ResetMatch();
        }
    }

    ///============TDocTrigramOverQueryCalcer===============
    ECalcer TDocTrigramOverQueryCalcer::GetCalcerType() const {
        return CalcerType;
    }

    void TDocTrigramOverQueryCalcer::NewQuery(const TString& query) {
        TCalcer::NewQuery(query);
        Query = query;
        State.NewQuery(query.size());
    }

    void TDocTrigramOverQueryCalcer::ProcessDoc(const char* docBeg, const char* docEnd) {
        Y_ASSERT(Inited);
        ::NSequences::TTrigramMatcher matcher(TString(docBeg, docEnd), SplitByWords);
        TTrigramCalcerCommon(matcher).UpdateBestMatch(Query.begin(), Query.end(), State);
    }

    void TDocTrigramOverQueryCalcer::Reset() {
        if (Inited) {
            State.ResetMatch();
        }
    }

}
