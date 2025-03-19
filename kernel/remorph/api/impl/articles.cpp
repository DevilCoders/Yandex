#include "articles.h"
#include "article.h"

#include <util/generic/utility.h>

namespace NRemorphAPI {

namespace NImpl {

TArticles::TArticles(TVector<NGzt::TArticlePtr>& articles)
{
    DoSwap(Articles, articles);
}

unsigned long TArticles::GetArticleCount() const {
    return Articles.size();
}

IArticle* TArticles::GetArticle(unsigned long num) const {
    return num < Articles.size() ? new TArticle(this, Articles[num]) : nullptr;
}

} // NImpl

} // NRemorphAPI
