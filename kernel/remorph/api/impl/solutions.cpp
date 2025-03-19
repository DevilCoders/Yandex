#include "solutions.h"
#include "fact.h"

#include <library/cpp/solve_ambig/unique_results.h>
#include <library/cpp/solve_ambig/find_solutions.h>

namespace NRemorphAPI {

namespace NImpl {

class TSolutionRef: public TBase, public IFacts {
private:
    const TVector<NFact::TFactPtr>& Facts;
    const NSolveAmbig::TSolution& Solution;
    const NText::TWordSymbols& Tokens;
    TLocker Lock;

public:
    TSolutionRef(const IBase* parent, const TVector<NFact::TFactPtr>& facts, const NSolveAmbig::TSolution& s, const NText::TWordSymbols& tokens)
        : Facts(facts)
        , Solution(s)
        , Tokens(tokens)
        , Lock(parent)
    {
    }

    unsigned long GetFactCount() const override {
        return Solution.Positions.size();
    }

    IFact* GetFact(unsigned long num) const override {
        Y_ASSERT(num >= Solution.Positions.size() || Solution.Positions[num] < Facts.size());
        return num < Solution.Positions.size() ? new TFact(this, *Facts[Solution.Positions[num]], Tokens) : nullptr;
    }
};


TSolutions::TSolutions(const IBase* parent, const TVector<NFact::TFactPtr>& facts, const NText::TWordSymbols& tokens, unsigned long limit, const NSolveAmbig::TRankMethod& rankMethod)
    : Facts(facts)
    , Tokens(tokens)
    , Lock(parent)
{
    NSolveAmbig::MakeUniqueResults(Facts);
    NSolveAmbig::FindAllSolutions(Facts, Solutions, rankMethod, limit);
}

unsigned long TSolutions::GetSolutionCount() const {
    return Solutions.size();
}

IFacts* TSolutions::GetSolution(unsigned long num) const {
    return num < Solutions.size() ? new TSolutionRef(this, Facts, *Solutions[num], Tokens) : nullptr;
}

} // NImpl

} // NRemorphAPI
