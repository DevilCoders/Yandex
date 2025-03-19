#pragma once

#include "base.h"

#include <kernel/remorph/facts/fact.h>
#include <kernel/remorph/text/word_input_symbol.h>

#include <util/generic/vector.h>

namespace NRemorphAPI {

namespace NImpl {

class TCompoundFields: public TBase, public ICompoundFields {
private:
    TVector<NFact::TCompoundFieldValuePtr> CompoundFields;
    const NText::TWordSymbols& Tokens;
    TLocker Lock;

public:
    TCompoundFields(const IBase* parent, TVector<NFact::TCompoundFieldValuePtr>& fields, const NText::TWordSymbols& tokens);

    unsigned long GetCompoundFieldCount() const override;
    ICompoundField* GetCompoundField(unsigned long num) const override;
};

} // NImpl

} // NRemorphAPI
