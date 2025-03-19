#pragma once

#include "range.h"

#include <kernel/remorph/facts/fact.h>
#include <kernel/remorph/text/word_input_symbol.h>

#include <util/generic/string.h>

namespace NRemorphAPI {

namespace NImpl {

class TField: public TBase, public TRangeBase, public virtual IArticles {
protected:
    const NFact::TFieldValue& Field;
    TString Value;
    TLocker Lock;

public:
    TField(const IBase* parent, const NFact::TFieldValue& f, const NText::TWordSymbols& tokens);

    // IRange
    IArticles* GetArticles() const override;
    bool HasArticle(const char* name) const override;

    // IField
    const char* GetName() const override;
    const char* GetValue() const override;

    // IArticles
    unsigned long GetArticleCount() const override;
    IArticle* GetArticle(unsigned long num) const override;
};

} // NImpl

} // NRemorphAPI
