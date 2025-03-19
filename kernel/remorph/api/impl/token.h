#pragma once

#include "base.h"

#include <kernel/remorph/text/word_input_symbol.h>

#include <util/generic/string.h>

namespace NRemorphAPI {

namespace NImpl {

class TToken: public TBase, public virtual IToken, public virtual IArticles {
private:
    TLocker Lock;
    const NText::TWordInputSymbol& Token;
    TString Text;

public:
    TToken(const IBase* parent, const NText::TWordInputSymbol& t);

    // IToken
    unsigned long GetStartSentPos() const override;
    unsigned long GetEndSentPos() const override;
    IArticles* GetArticles() const override;
    bool HasArticle(const char* name) const override;
    const char* GetText() const override;

    // IArticles
    unsigned long GetArticleCount() const override;
    IArticle* GetArticle(unsigned long num) const override;
};


} // NImpl

} // NRemorphAPI
