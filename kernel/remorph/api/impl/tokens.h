#pragma once

#include "base.h"

#include <kernel/remorph/text/word_input_symbol.h>

namespace NRemorphAPI {

namespace NImpl {

class TTokens: public TBase, public ITokens {
private:
    TLocker Lock;
    const NText::TWordSymbols& Tokens;

public:
    TTokens(const IBase* parent, const NText::TWordSymbols& tokens);

    unsigned long GetTokenCount() const override;
    IToken* GetToken(unsigned long num) const override;
};

} // NImpl

} // NRemorphAPI
