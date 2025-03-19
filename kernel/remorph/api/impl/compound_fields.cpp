#include "compound_fields.h"
#include "compound_field.h"

#include <util/generic/utility.h>

namespace NRemorphAPI {

namespace NImpl {

TCompoundFields::TCompoundFields(const IBase* parent, TVector<NFact::TCompoundFieldValuePtr>& fields, const NText::TWordSymbols& tokens)
    : Tokens(tokens)
    , Lock(parent)
{
    DoSwap(CompoundFields, fields);
}

unsigned long TCompoundFields::GetCompoundFieldCount() const {
    return CompoundFields.size();
}

ICompoundField* TCompoundFields::GetCompoundField(unsigned long num) const {
    return num < CompoundFields.size() ? new TCompoundField(this, *CompoundFields[num], Tokens) : nullptr;
}


} // NImpl

} // NRemorphAPI
