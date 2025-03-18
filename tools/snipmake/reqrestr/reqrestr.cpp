#include "reqrestr.h"

#include <kernel/qtree/richrequest/richnode.h>

#include <util/string/split.h>
#include <util/string/builder.h>

namespace NSnippets {

static TString AddRestr(const TString& qtree, const TRichNodePtr& n) {
    TBinaryRichTree b = DecodeRichTreeBase64(qtree);
    NSearchQuery::TRequest r;
    r.Deserialize(b);
    r.Root->AddRestrDocFilter(n);
    r.Serialize(b);
    return EncodeRichTreeBase64(b, true);
}
static void AddQtreeRestr(TCgiParameters& cgi, const TRichNodePtr& n) {
    TString qtree = cgi.Get("qtree");
    if (!qtree.size()) {
        return;
    }
    qtree = AddRestr(qtree, n);
    cgi.EraseAll("qtree");
    cgi.InsertUnescaped("qtree", qtree);
}
static void AddForeignQtreeRestr(TCgiParameters& cgi, const TRichNodePtr& n) {
    TString relev = cgi.Get("relev");
    if (!relev.size()) {
        return;
    }
    TVector<TString> rx;
    TCharDelimiter<const char> d(';');
    TContainerConsumer< TVector<TString> > c(&rx);
    SplitString(relev.data(), relev.data() + relev.size(), d, c);
    relev = "";
    for (size_t i = 0; i < rx.size(); ++i) {
        if (i)
            relev += ';';
        if (rx[i].StartsWith("qtree_for_foreign=")) {
            relev += TString("qtree_for_foreign=") + AddRestr(rx[i].substr(sizeof("qtree_for_foreign=") - 1), n);
        } else {
            relev += rx[i];
        }
    }
    cgi.EraseAll("relev");
    cgi.InsertUnescaped("relev", relev);
}
void AddRestrToQtrees(TCgiParameters& cgi, const TString& restr) {
    TRichNodePtr n = CreateRichNode(UTF8ToWide(restr), TCreateTreeOptions());
    AddQtreeRestr(cgi, n);
    AddForeignQtreeRestr(cgi, n);
}
void AddRestrToQtrees(TCgiParameters& cgi, const TVector<TString>& urls) {
    TStringBuilder restr;
    restr << "(";
    for (size_t i = 0; i < urls.size(); ++i) {
        if (i) {
            restr << " | ";
        }
        restr << "url:\"" << urls[i] << "\"";
    }
    restr << ")";
    AddRestrToQtrees(cgi, restr);
}
}
