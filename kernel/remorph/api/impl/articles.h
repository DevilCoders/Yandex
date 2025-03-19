#pragma once

#include "base.h"

#include <kernel/gazetteer/gazetteer.h>

#include <util/generic/vector.h>

namespace NRemorphAPI {

namespace NImpl {

class TArticles: public TBase, public IArticles {
private:
    TVector<NGzt::TArticlePtr> Articles;

public:
    TArticles(TVector<NGzt::TArticlePtr>& articles);

    unsigned long GetArticleCount() const override;
    IArticle* GetArticle(unsigned long num) const override;
};


} // NImpl

} // NRemorphAPI
