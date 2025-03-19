#pragma once

#include "range.h"
#include "field_container.h"

#include <kernel/remorph/facts/fact.h>
#include <kernel/remorph/text/word_input_symbol.h>

#include <util/generic/string.h>

namespace NRemorphAPI {

namespace NImpl {

class TFact: public TBase, public TRangeBase, public TFieldContainerBase, public virtual IFact {
private:
    const NFact::TFact& Fact;
    TString Value;
    TLocker Lock;

public:
    TFact(const IBase* parent, const NFact::TFact& f, const NText::TWordSymbols& tokens);

    // IField (unimplemented from TRangeBase)
    const char* GetName() const override;
    const char* GetValue() const override;

    // IFact
    const char* GetType() const override;
    unsigned long GetCoverage() const override;
};

} // NImpl

} // NRemorphAPI
