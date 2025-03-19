#include "repr.h"

#include <kernel/gazetteer/gazetteer.h>

namespace NSymbol {

void TInputSymbolRepr::Label(IOutputStream& output, const TInputSymbol& symbol) {
    output << symbol.GetText();
}

void TInputSymbolRepr::DotPayload(IOutputStream& output, const TInputSymbol& symbol, const TString& symbolId) {
    const TVector<NGzt::TArticlePtr>& articles = symbol.GetGztArticles();
    size_t n = 0;
    for (TVector<NGzt::TArticlePtr>::const_iterator iArticle = articles.begin(); iArticle != articles.end(); ++iArticle) {
        ++n;
        TString id = symbolId + "gzt" + ::ToString(n);
        output << "\"" << id << "\" [label=\"@" << iArticle->GetTitle() << "\",shapre=signature,style=dashed,color=lightgrey,fontsize=10,group=grp" << id << "];" << Endl;
        output << "\"" << symbolId << "\" -> \"" << id << "\" [style=dashed,color=lightgrey];" << Endl;
    }
}

void TInputSymbolRepr::DotAttrsPayload(IOutputStream& output, const TInputSymbol& symbol, const TString& symbolId) {
    Y_UNUSED(symbol);

    output << "group=grp" << symbolId;
}

}
