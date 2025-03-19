#include "facts.h"
#include "fact.h"

#include <util/generic/utility.h>

namespace NRemorphAPI {

namespace NImpl {

TFacts::TFacts(const IBase* parent, TVector<NFact::TFactPtr>& facts, const NText::TWordSymbols& tokens)
    : Tokens(tokens)
    , Lock(parent)
{
    DoSwap(Facts, facts);
}

unsigned long TFacts::GetFactCount() const {
    return Facts.size();
}

IFact* TFacts::GetFact(unsigned long num) const {
    return num < Facts.size() ? new TFact(this, *Facts[num], Tokens) : nullptr;
}

} // NImpl

} // NRemorphAPI
