#pragma once

#include "base.h"

#include <kernel/remorph/facts/fact.h>
#include <kernel/remorph/text/word_input_symbol.h>

#include <library/cpp/solve_ambig/rank.h>
#include <library/cpp/solve_ambig/solution.h>

#include <util/generic/vector.h>

namespace NRemorphAPI {

namespace NImpl {

class TSolutions: public TBase, public ISolutions {
private:
    TVector<NFact::TFactPtr> Facts;
    TVector<NSolveAmbig::TSolutionPtr> Solutions;
    const NText::TWordSymbols& Tokens;
    TLocker Lock;

public:
    TSolutions(const IBase* parent, const TVector<NFact::TFactPtr>& facts, const NText::TWordSymbols& tokens, unsigned long limit, const NSolveAmbig::TRankMethod& rankMethod);

    unsigned long GetSolutionCount() const override;
    IFacts* GetSolution(unsigned long num) const override;
};


} // NImpl

} // NRemorphAPI
