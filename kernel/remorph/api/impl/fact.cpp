#include "fact.h"

#include <kernel/remorph/input/input_symbol_util.h>
#include <kernel/remorph/facts/facttype.h>

namespace NRemorphAPI {

namespace NImpl {

TFact::TFact(const IBase* parent, const NFact::TFact& f, const NText::TWordSymbols& tokens)
    : TRangeBase(f, tokens)
    , TFieldContainerBase(f, tokens)
    , Fact(f)
    , Lock(parent)
{
    Y_ASSERT(f.GetSrcPos().first < tokens.size());
    Y_ASSERT(f.GetSrcPos().second <= tokens.size());
    Value = NSymbol::ToString(tokens.begin() + f.GetSrcPos().first, tokens.begin() + f.GetSrcPos().second);
}

const char* TFact::GetName() const {
    return GetType();
}

const char* TFact::GetValue() const {
    return Value.data();
}

const char* TFact::GetType() const {
    return Fact.GetType().GetTypeName().data();
}

unsigned long TFact::GetCoverage() const {
    return Fact.GetCoverage();
}


} // NImpl

} // NRemorphAPI
