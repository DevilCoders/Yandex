#include "compound_field.h"

#include <util/charset/wide.h>

namespace NRemorphAPI {

namespace NImpl {

TCompoundField::TCompoundField(const IBase* parent, const NFact::TCompoundFieldValue& f, const NText::TWordSymbols& tokens)
    : TRangeBase(f, tokens)
    , TFieldContainerBase(f, tokens)
    , CompoundField(f)
    , Lock(parent)
{
    Value = WideToUTF8(CompoundField.GetText());
}

const char* TCompoundField::GetName() const {
    return CompoundField.GetName().data();
}

const char* TCompoundField::GetValue() const {
    return Value.data();
}

} // NImpl

} // NRemorphAPI
