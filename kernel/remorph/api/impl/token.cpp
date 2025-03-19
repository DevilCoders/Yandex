#include "token.h"
#include "article.h"

#include <util/charset/wide.h>
#include <util/generic/bitmap.h>

namespace NRemorphAPI {

namespace NImpl {

TToken::TToken(const IBase* parent, const NText::TWordInputSymbol& t)
    : Lock(parent)
    , Token(t)
{
    Text = WideToUTF8(Token.GetText());
}

unsigned long TToken::GetStartSentPos() const {
    return Token.GetSentencePos().first;
}

unsigned long TToken::GetEndSentPos() const {
    return Token.GetSentencePos().second;
}

IArticles* TToken::GetArticles() const {
    IArticles* articles = const_cast<TToken*>(this);
    articles->AddRef();
    return articles;
}

bool TToken::HasArticle(const char* name) const {
    TDynBitMap ctx;
    return Token.HasGztArticle(UTF8ToWide(name), ctx);
}

const char* TToken::GetText() const {
    return Text.data();
}

unsigned long TToken::GetArticleCount() const {
    return Token.GetGztArticles().size();
}

IArticle* TToken::GetArticle(unsigned long num) const {
    return num < Token.GetGztArticles().size() ? new TArticle(this, Token.GetGztArticles()[num]) : nullptr;
}

} // NImpl

} // NRemorphAPI

