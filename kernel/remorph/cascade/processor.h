#pragma once

#include "cascade.h"

#include <kernel/gazetteer/gazetteer.h>
#include <kernel/remorph/core/core.h>
#include <kernel/remorph/proc_base/matcher_base.h>

#include <library/cpp/solve_ambig/solve_ambiguity.h>
#include <library/cpp/solve_ambig/rank.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/string/vector.h>

namespace NCascade {

template <class TRootMatcher>
class TProcessor: public TCascade<TRootMatcher> {
public:
    typedef typename TCascade<TRootMatcher>::TResult TResult;
    typedef typename TCascade<TRootMatcher>::TResultPtr TResultPtr;
    typedef typename TCascade<TRootMatcher>::TResults TResults;

    typedef TIntrusivePtr<TProcessor<TRootMatcher>> TPtr;

private:
    bool ResolveResultAmbiguity = false;
    NSolveAmbig::TRankMethod ResultRankMethod{NSolveAmbig::DefaultRankMethod()};
    NMatcher::ESearchMethod Mode = NMatcher::SM_SEARCH_ALL;

public:
    TProcessor(const TString& filePath, const NGzt::TGazetteer* gzt) {
        TCascade<TRootMatcher>::LoadRootCascadeFromFile(filePath, gzt);
    }

    TProcessor(const TString& filePath, const NGzt::TGazetteer* gzt, const TString& baseDir) {
        TCascade<TRootMatcher>::LoadRootCascadeFromFile(filePath, gzt, baseDir);
    }

    TProcessor(IInputStream& data, const NGzt::TGazetteer* gzt) {
        TCascade<TRootMatcher>::LoadRootCascadeFromStream(data, gzt);
    }

    inline void SetResolveResultAmbiguity(bool resolve) {
        ResolveResultAmbiguity = resolve;
    }

    inline bool GetResolveResultAmbiguity() const {
        return ResolveResultAmbiguity;
    }

    inline void SetResultRankMethod(const NSolveAmbig::TRankMethod& rankMethod) {
        ResultRankMethod = rankMethod;
    }

    inline const NSolveAmbig::TRankMethod& GetResultRankMethod() const {
        return ResultRankMethod;
    }

    inline void SetMode(NMatcher::ESearchMethod mode) {
        Mode = mode;
    }

    template <class TSymbolPtr>
    void Process(NRemorph::TInput<TSymbolPtr>& input, TResults& results, NRemorph::OperationCounter* opcnt = nullptr) const {
        TResultPtr res;
        switch (Mode) {
        case NMatcher::SM_SEARCH:
            res = TCascade<TRootMatcher>::Search(input, opcnt);
            break;
        case NMatcher::SM_MATCH:
            res = TCascade<TRootMatcher>::Match(input, opcnt);
            break;
        case NMatcher::SM_SEARCH_ALL:
            TCascade<TRootMatcher>::SearchAll(input, results, opcnt);
            break;
        case NMatcher::SM_MATCH_ALL:
            TCascade<TRootMatcher>::MatchAll(input, results, opcnt);
            break;
        default:
            Y_FAIL("Unimplemented search method");
        }
        if (res) {
            results.push_back(res);
        }
        if (ResolveResultAmbiguity && results.size() > 1) {
            NSolveAmbig::SolveAmbiguity(results, ResultRankMethod);
        }
    }

    template <class TSymbolPtr>
    inline void Process(const TVector<TSymbolPtr>& symbols, const TGazetteer* gzt,
        NRemorph::TInput<TSymbolPtr>& input, TResults& results, NRemorph::OperationCounter* opcnt = nullptr) const {
        if (TCascade<TRootMatcher>::CreateInput(input, symbols, gzt))
            Process(input, results, opcnt);
    }

    template <class TSymbolPtr, class TGztResultIterator>
    inline void Process(const TVector<TSymbolPtr>& symbols, TGztResultIterator& iter,
        NRemorph::TInput<TSymbolPtr>& input, TResults& results, NRemorph::OperationCounter* opcnt = nullptr) const {
        if (TCascade<TRootMatcher>::CreateInput(input, symbols, iter))
            Process(input, results, opcnt);
    }
};

} // NCascade
