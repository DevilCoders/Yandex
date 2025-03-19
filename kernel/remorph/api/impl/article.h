#pragma once

#include "base.h"

#include <kernel/gazetteer/gazetteer.h>

#include <util/generic/string.h>

namespace NRemorphAPI {

namespace NImpl {

class TArticle: public TBase, public IArticle {
private:
    TLocker Lock;
    const NGzt::TArticlePtr& Article;
    TString Title;

public:
    TArticle(const IBase* parent, const NGzt::TArticlePtr& a);

    const char* GetType() const override;
    const char* GetName() const override;
    IBlob* GetBlob() const override;
    IBlob* GetJsonBlob() const override;
};


} // NImpl

} // NRemorphAPI
