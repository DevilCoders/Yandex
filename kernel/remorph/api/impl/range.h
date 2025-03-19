#pragma once

#include "base.h"

#include <kernel/remorph/facts/fact.h>
#include <kernel/remorph/text/word_input_symbol.h>

#include <util/generic/string.h>

namespace NRemorphAPI {

namespace NImpl {

class TRangeBase: public virtual IField, public virtual ITokens {
protected:
    const NFact::TRangeInfo& Range;
    const NText::TWordSymbols& Tokens;

protected:
    inline unsigned long GetRangeSize() const {
        return Range.GetSrcPos().second - Range.GetSrcPos().first;
    }

public:
    TRangeBase(const NFact::TRangeInfo& r, const NText::TWordSymbols& tokens);

    // IRange
    unsigned long GetStartSentPos() const override;
    unsigned long GetEndSentPos() const override;

    // IField
    unsigned long GetStartToken() const override;
    unsigned long GetEndToken() const override;
    ITokens* GetTokens() const override;

    // ITokens
    unsigned long GetTokenCount() const override;
    IToken* GetToken(unsigned long num) const override;
};

} // NImpl

} // NRemorphAPI
