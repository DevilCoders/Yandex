#include "tokens.h"
#include "token.h"

namespace NRemorphAPI {

namespace NImpl {

TTokens::TTokens(const IBase* parent, const NText::TWordSymbols& tokens)
    : Lock(parent)
    , Tokens(tokens)
{
}

unsigned long TTokens::GetTokenCount() const {
    return Tokens.size();
}

IToken* TTokens::GetToken(unsigned long num) const {
    return num < Tokens.size() ? new TToken(this, *Tokens[num]) : nullptr;
}

} // NImpl

} // NRemorphAPI

