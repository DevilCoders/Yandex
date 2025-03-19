#pragma once

#include "field.h"

#include <kernel/remorph/facts/fact.h>
#include <kernel/remorph/text/word_input_symbol.h>

#include <util/generic/string.h>

namespace NRemorphAPI {

namespace NImpl {

class TFieldContainerBase: public virtual ICompoundField {
private:
    const NFact::TFieldValueContainer& Container;
    const NText::TWordSymbols& Tokens;

public:
    TFieldContainerBase(const NFact::TFieldValueContainer& c, const NText::TWordSymbols& tokens);

    // IRange
    IArticles* GetArticles() const override;
    bool HasArticle(const char* name) const override;

    // ICompoundField
    const char* GetRule() const override;
    double GetWeight() const override;
    IFields* GetFields() const override;
    IFields* GetFields(const char* fieldName) const override;
    ICompoundFields* GetCompoundFields() const override;
    ICompoundFields* GetCompoundFields(const char* fieldName) const override;
};

} // NImpl

} // NRemorphAPI
