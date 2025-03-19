#pragma once

#include "base.h"
#include "range.h"
#include "field_container.h"

#include <kernel/remorph/facts/fact.h>
#include <kernel/remorph/text/word_input_symbol.h>

#include <util/generic/string.h>

namespace NRemorphAPI {

namespace NImpl {

class TCompoundField: public TBase, public TRangeBase, public TFieldContainerBase {
private:
    const NFact::TCompoundFieldValue& CompoundField;
    TString Value;
    TLocker Lock;

public:
    TCompoundField(const IBase* parent, const NFact::TCompoundFieldValue& f, const NText::TWordSymbols& tokens);

    // IField
    const char* GetName() const override;
    const char* GetValue() const override;
};

} // NImpl

} // NRemorphAPI
