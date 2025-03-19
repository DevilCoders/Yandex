#pragma once

#include "base.h"

#include <kernel/remorph/facts/fact.h>
#include <kernel/remorph/text/word_input_symbol.h>

#include <util/generic/vector.h>

namespace NRemorphAPI {

namespace NImpl {

class TFields: public TBase, public IFields {
private:
    TVector<NFact::TFieldValuePtr> Fields;
    const NText::TWordSymbols& Tokens;
    TLocker Lock;

public:
    TFields(const IBase* parent, TVector<NFact::TFieldValuePtr>& fields, const NText::TWordSymbols& tokens);

    unsigned long GetFieldCount() const override;
    IField* GetField(unsigned long num) const override;
};

} // NImpl

} // NRemorphAPI
