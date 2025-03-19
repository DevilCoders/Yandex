#include "sentence.h"
#include "tokens.h"
#include "facts.h"
#include "solutions.h"
#include "fact.h"

#include <library/cpp/solve_ambig/solve_ambiguity.h>

#include <util/generic/utility.h>

namespace NRemorphAPI {

namespace NImpl {

namespace {

inline NSolveAmbig::ERankCheck GetImplValue(ERankCheck rankCheck) {
    switch (rankCheck) {
        case RC_G_COVERAGE:
            return NSolveAmbig::RC_G_COVERAGE;
        case RC_L_COUNT:
            return NSolveAmbig::RC_L_COUNT;
        case RC_G_WEIGHT:
            return NSolveAmbig::RC_G_WEIGHT;
        default:
            throw yexception() << "Unsupported ranking check";
    }
}

inline void FillRankMethod(NSolveAmbig::TRankMethod& rankMethod, const ERankCheck* rankChecks, unsigned int rankChecksNum) {
    if ((rankChecks == nullptr) || (rankChecksNum == 0)) {
        throw yexception() << "Non-empty ranking checks sequence must be specified (or use default-ranking method variant)";
    }

    for (unsigned int i = 0; i < rankChecksNum; ++i) {
        rankMethod.push_back(GetImplValue(rankChecks[i]));
    }
}

}

TSentence::TSentence(const IBase* parent, const TSentenceData& data)
    : Data(data)
    , Lock(parent)
{
}

const char* TSentence::GetText() const {
    return Data.Text.data();
}

ITokens* TSentence::GetTokens() const {
    return new TTokens(this, Data.Tokens);
}

IFacts* TSentence::GetAllFacts() const {
    IFacts* facts = const_cast<TSentence*>(this);
    facts->AddRef();
    return facts;
}

IFacts* TSentence::FindBestSolution() const {
    return FindBestSolution(NSolveAmbig::DefaultRankMethod());
}

IFacts* TSentence::FindBestSolution(const ERankCheck* rankChecks, unsigned int rankChecksNum) const {
    NSolveAmbig::TRankMethod rankMethod;
    FillRankMethod(rankMethod, rankChecks, rankChecksNum);
    return FindBestSolution(rankMethod);
}

ISolutions* TSentence::FindAllSolutions(unsigned short maxSolutions) const {
    return FindAllSolutions(maxSolutions, NSolveAmbig::DefaultRankMethod());
}

ISolutions* TSentence::FindAllSolutions(unsigned short maxSolutions, const ERankCheck* rankChecks, unsigned int rankChecksNum) const {
    NSolveAmbig::TRankMethod rankMethod;
    FillRankMethod(rankMethod, rankChecks, rankChecksNum);
    return FindAllSolutions(maxSolutions, rankMethod);
}

unsigned long TSentence::GetFactCount() const {
    return Data.Facts.size();
}

IFact* TSentence::GetFact(unsigned long num) const {
    return num < Data.Facts.size() ? new TFact(this, *Data.Facts[num], Data.Tokens) : nullptr;
}

IFacts* TSentence::FindBestSolution(const NSolveAmbig::TRankMethod& rankMethod) const {
    TVector<NFact::TFactPtr> bestFacts(Data.Facts);
    NSolveAmbig::SolveAmbiguity(bestFacts, rankMethod);
    return new TFacts(this, bestFacts, Data.Tokens);
}

ISolutions* TSentence::FindAllSolutions(unsigned short maxSolutions, const NSolveAmbig::TRankMethod& rankMethod) const {
    return new TSolutions(this, Data.Facts, Data.Tokens, maxSolutions, rankMethod);
}

} // NImpl

} // NRemorphAPI
