#pragma once

#include "base.h"

#include <kernel/remorph/facts/fact.h>
#include <kernel/remorph/text/word_input_symbol.h>

#include <util/generic/vector.h>

namespace NRemorphAPI {

namespace NImpl {

class TFacts: public TBase, public IFacts {
private:
    TVector<NFact::TFactPtr> Facts;
    const NText::TWordSymbols& Tokens;
    TLocker Lock;

public:
    TFacts(const IBase* parent, TVector<NFact::TFactPtr>& facts, const NText::TWordSymbols& tokens);

    unsigned long GetFactCount() const override;
    IFact* GetFact(unsigned long num) const override;
};


} // NImpl

} // NRemorphAPI
