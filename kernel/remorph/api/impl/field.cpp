#include "field.h"
#include "article.h"
#include "token.h"

#include <util/charset/wide.h>

namespace NRemorphAPI {

namespace NImpl {

TField::TField(const IBase* parent, const NFact::TFieldValue& f, const NText::TWordSymbols& tokens)
    : TRangeBase(f, tokens)
    , Field(f)
    , Value(WideToUTF8(Field.GetText()))
    , Lock(parent)
{
}

IArticles* TField::GetArticles() const {
    IArticles* articles = const_cast<TField*>(this);
    articles->AddRef();
    return articles;
}

bool TField::HasArticle(const char* name) const {
    const TUtf16String wname = UTF8ToWide(name);
    for (TVector<NGzt::TArticlePtr>::const_iterator i = Field.GetArticles().begin(); i != Field.GetArticles().end(); ++i) {
        if (i->GetTitle() == wname || i->GetTypeName() == wname) {
            return true;
        }
    }
    return false;
}

const char* TField::GetName() const {
    return Field.GetName().data();
}

const char* TField::GetValue() const {
    return Value.data();
}

unsigned long TField::GetArticleCount() const {
    return Field.GetArticles().size();
}

IArticle* TField::GetArticle(unsigned long num) const {
    return num < Field.GetArticles().size() ? new TArticle(this, Field.GetArticles()[num]) : nullptr;
}


} // NImpl

} // NRemorphAPI
