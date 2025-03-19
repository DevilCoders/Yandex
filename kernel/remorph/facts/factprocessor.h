#pragma once

#include "type_database.h"
#include "fact.h"

#include <kernel/remorph/cascade/processor.h>
#include <kernel/remorph/matcher/matcher.h>
#include <kernel/remorph/tokenlogic/tlmatcher.h>
#include <kernel/remorph/engine/char/char_engine.h>
#include <kernel/remorph/common/verbose.h>

#include <kernel/gazetteer/gazetteer.h>
#include <kernel/geograph/geograph.h>

#include <library/cpp/solve_ambig/solve_ambiguity.h>
#include <library/cpp/solve_ambig/rank.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/generic/typetraits.h>

#include <utility>

namespace NFact {

class TFactProcessor: public TFactTypeDatabase, public TAtomicRefCount<TFactProcessor> {
private:
    typedef TSimpleSharedPtr<const NGzt::TGazetteer> TGazetteerPtr;
    typedef TSimpleSharedPtr<const NGeoGraph::TGeoGraph> TGeoGraphPtr;
    typedef TIntrusivePtr<NCascade::TProcessor<NReMorph::TMatcher>> TRemorphPtr;
    typedef TIntrusivePtr<NCascade::TProcessor<NTokenLogic::TMatcher>> TTokenLogicPtr;
    typedef TIntrusivePtr<NCascade::TProcessor<NReMorph::TCharEngine>> TCharEnginePtr;

private:
    THashMap<TString, TGazetteerPtr> Gazetteers;
    TVector<TGeoGraphPtr> GeoGraphs;
    TVector<std::pair<const TGazetteer*, TRemorphPtr>> Remorphs;
    TVector<std::pair<const TGazetteer*, TTokenLogicPtr>> TokenLogics;
    TVector<std::pair<const TGazetteer*, TCharEnginePtr>> CharEngines;

    bool ResolveFactAmbiguity = true;
    NSolveAmbig::TRankMethod FactRankMethod{NSolveAmbig::DefaultRankMethod()};

private:
    struct TReportDeletedFact {
        Y_FORCE_INLINE void operator() (const TFactPtr& fact) const {
            REPORT(NOTICE, "Deleting ambiguous fact: rule=" << fact->GetRuleName()
                << ", w=" << fact->GetWeight()
                << ": " << fact->ToString(false));
        }
    };

    template <class TMatcherPtr, class TSymbolPtr>
    void CollectFacts(const TVector<std::pair<const TGazetteer*, TMatcherPtr>>& matchers,
        const TVector<TSymbolPtr>& symbols, TVector<NFact::TFactPtr>& facts,
        NRemorph::OperationCounter* opcnt) const {

        typedef typename TMatcherPtr::TValueType TMatcher;
        typedef typename TMatcher::TResults TResults;

        NRemorph::TInput<TSymbolPtr> input;
        TResults results;
        for (size_t i = 0; i < matchers.size(); ++i) {
            results.clear();
            input.Clear();
            matchers[i].second->Process(symbols, matchers[i].first, input, results, opcnt);
            if (!results.empty())
                GetFactTypes()[i]->ResultsToFacts(input, results, facts);
        }
    }

    template <typename TMatcherPtr, class TSymbolPtr, class TGztResultIterator>
    void CollectFacts(const TVector<std::pair<const TGazetteer*, TMatcherPtr>>& matchers,
        const TVector<TSymbolPtr>& symbols, TGztResultIterator& iter, TVector<NFact::TFactPtr>& facts,
        NRemorph::OperationCounter* opcnt) const {

        typedef typename TMatcherPtr::TValueType TMatcher;
        typedef typename TMatcher::TResults TResults;

        NRemorph::TInput<TSymbolPtr> input;
        TResults results;
        for (size_t i = 0; i < matchers.size(); ++i) {
            results.clear();
            input.Clear();
            matchers[i].second->Process(symbols, iter, input, results, opcnt);
            if (!results.empty())
                GetFactTypes()[i]->ResultsToFacts(input, results, facts);
            iter.Reset();
        }
    }

    void Reset();

    template <class TMatcher>
    void InitMatcher(const TFactType& factType, const TGazetteer* gzt, TMatcher& matcher);

public:
    TFactProcessor() = default;
    TFactProcessor(const TString& path, const TGazetteer* externalGzt = nullptr) {
        Init(path, externalGzt);
    }

    ~TFactProcessor() override {
    }

    void AddFactType(const NProtoBuf::Descriptor& descr, IInputStream& cascade, const TGazetteer* gzt);

    inline bool GetResolveFactAmbiguity() const {
        return ResolveFactAmbiguity;
    }

    inline void SetResolveFactAmbiguity(bool resolveFactAmbiguity) {
        ResolveFactAmbiguity = resolveFactAmbiguity;
    }

    inline const NSolveAmbig::TRankMethod& GetFactRankMethod() const {
        return FactRankMethod;
    }

    inline void SetFactRankMethod(const NSolveAmbig::TRankMethod& factRankMethod) {
        FactRankMethod = factRankMethod;
    }

    template <class TSymbolPtr>
    void CollectFacts(const TVector<TSymbolPtr>& symbols, TVector<NFact::TFactPtr>& facts,
                      NRemorph::OperationCounter* opcnt = nullptr) const {
        if (!Remorphs.empty())
            CollectFacts(Remorphs, symbols, facts, opcnt);
        if (!TokenLogics.empty())
            CollectFacts(TokenLogics, symbols, facts, opcnt);
        if (!CharEngines.empty())
            CollectFacts(CharEngines, symbols, facts, opcnt);

        if (ResolveFactAmbiguity && facts.size() > 1)
            NSolveAmbig::SolveAmbiguity(facts, TReportDeletedFact(), FactRankMethod);
    }

    template <class TSymbolPtr, class TGztResultIterator>
    void CollectFacts(const TVector<TSymbolPtr>& symbols, TGztResultIterator& iter, TVector<NFact::TFactPtr>& facts,
                      NRemorph::OperationCounter* opcnt = nullptr) const {
        if (!Remorphs.empty())
            CollectFacts(Remorphs, symbols, iter, facts, opcnt);
        if (!TokenLogics.empty())
            CollectFacts(TokenLogics, symbols, iter, facts, opcnt);
        if (!CharEngines.empty())
            CollectFacts(CharEngines, symbols, iter, facts, opcnt);

        if (ResolveFactAmbiguity && facts.size() > 1)
            NSolveAmbig::SolveAmbiguity(facts, TReportDeletedFact(), FactRankMethod);
    }

    void Init(const TString& path, const TGazetteer* externalGzt = nullptr);
};

typedef TIntrusivePtr<TFactProcessor> TFactProcessorPtr;

} // NFact
