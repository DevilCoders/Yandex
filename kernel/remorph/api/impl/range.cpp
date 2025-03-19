#include "range.h"
#include "token.h"

#include <util/charset/wide.h>

namespace NRemorphAPI {

namespace NImpl {

TRangeBase::TRangeBase(const NFact::TRangeInfo& r, const NText::TWordSymbols& tokens)
    : Range(r)
    , Tokens(tokens)
{
}

unsigned long TRangeBase::GetStartSentPos() const {
    Y_ASSERT(Range.GetSrcPos().first < Tokens.size());
    return Tokens[Range.GetSrcPos().first]->GetSentencePos().first;
}

unsigned long TRangeBase::GetEndSentPos() const {
    Y_ASSERT(Range.GetSrcPos().second <= Tokens.size());
    return Tokens[Range.GetSrcPos().second - 1]->GetSentencePos().second;
}

unsigned long TRangeBase::GetStartToken() const {
    return Range.GetSrcPos().first;
}

unsigned long TRangeBase::GetEndToken() const {
    return Range.GetSrcPos().second;
}

ITokens* TRangeBase::GetTokens() const {
    ITokens* tokens = const_cast<TRangeBase*>(this);
    tokens->AddRef();
    return tokens;
}

unsigned long TRangeBase::GetTokenCount() const {
    return GetRangeSize();
}

IToken* TRangeBase::GetToken(unsigned long num) const {
    Y_ASSERT(Range.GetSrcPos().first + num < Tokens.size());
    return num < GetRangeSize() ? new TToken(this, *Tokens[Range.GetSrcPos().first + num]) : nullptr;
}

} // NImpl

} // NRemorphAPI
