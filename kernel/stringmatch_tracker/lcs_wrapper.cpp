#include "lcs_wrapper.h"

namespace NStringMatchTracker {

    ECalcer TLCSCalcer::GetCalcerType() const {
        return CalcerType;
    }

    void TLCSCalcer::NewQuery(const TString& query) {
        TCalcer::NewQuery(query);
        Matcher.Reset(new ::NSequences::TLCSMatcher(query, Threshold));
        State.NewQuery(query.size());
        Matcher->ResetMatch();
    }

    void TLCSCalcer::ProcessDoc(const char* docBeg, const char* docEnd) {
        Y_ASSERT(Matcher);
        Y_ASSERT(Inited);
        Y_ASSERT(docEnd - docBeg >= 0);
        Matcher->ResetMatch();

        for (auto it = docEnd; it != docBeg; --it) {
            Matcher->ExtendMatch(*(it - 1));
        }

        if (Matcher->GetCurMatch() == State.BestMatch) {
            State.BestDocLen = Min(State.BestDocLen, size_t(docEnd - docBeg));
        } else if (Matcher->GetCurMatch() > State.BestMatch) {
            State.BestDocLen = size_t(docEnd - docBeg);
            State.BestMatch = Matcher->GetCurMatch();
        }
    }

    void TLCSCalcer::Reset() {
        if (Inited) {
            State.ResetMatch();
            Matcher->ResetMatch();
        }
    }

}
