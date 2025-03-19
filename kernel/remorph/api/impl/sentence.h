#pragma once

#include "base.h"

#include <kernel/remorph/facts/fact.h>
#include <kernel/remorph/text/word_input_symbol.h>

#include <library/cpp/solve_ambig/rank.h>

#include <util/generic/vector.h>

namespace NRemorphAPI {

namespace NImpl {

struct TSentenceData: public TSimpleRefCount<TSentenceData> {
    NText::TWordSymbols Tokens;
    TVector<NFact::TFactPtr> Facts;
    TString Text;
};

class TSentence: public TBase, public virtual ISentence, public virtual IFacts {
private:
    const TSentenceData& Data;
    TLocker Lock;

public:
    TSentence(const IBase* parent, const TSentenceData& data);

    // ISentence
    const char* GetText() const override;
    ITokens* GetTokens() const override;
    IFacts* GetAllFacts() const override;
    IFacts* FindBestSolution() const override;
    IFacts* FindBestSolution(const ERankCheck* rankChecks, unsigned int rankChecksNum) const override;
    ISolutions* FindAllSolutions(unsigned short maxSolutions) const override;
    ISolutions* FindAllSolutions(unsigned short maxSolutions, const ERankCheck* rankChecks, unsigned int rankChecksNum) const override;

    // IFacts
    unsigned long GetFactCount() const override;
    IFact* GetFact(unsigned long num) const override;

private:
    IFacts* FindBestSolution(const NSolveAmbig::TRankMethod& rankMethod) const;
    ISolutions* FindAllSolutions(unsigned short maxSolutions, const NSolveAmbig::TRankMethod& rankMethod) const;
};

} // NImpl

} // NRemorphAPI
