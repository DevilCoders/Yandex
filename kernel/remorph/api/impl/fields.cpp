#include "fields.h"
#include "field.h"

#include <util/generic/utility.h>

namespace NRemorphAPI {

namespace NImpl {

TFields::TFields(const IBase* parent, TVector<NFact::TFieldValuePtr>& fields, const NText::TWordSymbols& tokens)
    : Tokens(tokens)
    , Lock(parent)
{
    DoSwap(Fields, fields);
}

unsigned long TFields::GetFieldCount() const {
    return Fields.size();
}

IField* TFields::GetField(unsigned long num) const {
    return num < Fields.size() ? new TField(this, *Fields[num], Tokens) : nullptr;
}


} // NImpl

} // NRemorphAPI
