#include "xmlsearchin.h"

#include <tools/snipmake/snipdat/xmlsearchin.xsyn.h>

#include <library/cpp/uri/http_url.h>

#include <library/cpp/cgiparam/cgiparam.h>

namespace NSnippets {
namespace NXmlSearchIn {

static TString GetQtree(const TString& url) {
    size_t p = url.find("&qtree=");
    if (p == TString::npos) {
        return TString();
    }
    p += 7;
    size_t pp = p + 1;
    while (pp < url.size() && url[pp] != '&') {
        ++pp;
    }
    TString res = url.substr(p, pp - p);
    CGIUnescape(res);
    return res;
}

struct TXmlsearchinWrap {
    TRequest* Res;
    TString Text;

    TXmlsearchinWrap()
      : Res(nullptr)
    {
    }

    void SetQuery(const char* text, size_t len) {
        Res->QueryUTF8.append(text, len);
    }
    void BeginFullQuery() {
        Res->FullQuery.clear();
    }
    void SetFullQuery(const char* text, size_t len) {
        Res->FullQuery.append(text, len);
    }
    void DoneFullQuery() {
        Res->Qtree = GetQtree(Res->FullQuery);
    }
    void DocumentBegin() {
        Res->Documents.emplace_back();
    }
    void SetText(const char* text, size_t len) {
        Text.append(text, len);
    }
    void BeginHilitedUrl() {
        Text.clear();
    }
    void EndHilitedUrl() {
        Res->Documents.back().HilitedUrl = Text;
    }
    void SetUrlmenu(const char* text, size_t len) {
        Res->Documents.back().SerializedUrlmenu.append(text, len);
    }
    void AddSearchHttpUrl(const char* text, size_t len) {
        Res->Documents.back().SearchHttpUrl.push_back(TString(text, len));
    }
    void BeginHilite() {
        static const TString openHlMarker = "\x07[";
        Text += openHlMarker;
    }
    void EndHilite() {
        static const TString closeHlMarker = "\x07]";
        Text += closeHlMarker;
    }

    void ErrMessage(ELogPriority /*priority*/, const char* /*fmt*/, ...) {
        //screw this, unknown tags/attrs get us here
    }
};

bool ParseRequest(TRequest& res, TStringBuf s) {
    if (!s) {
        Cerr << "empty xmlsearch response" << Endl;
        return false;
    }
    TXmlSaxParser< TXmlsearchinParser<TXmlsearchinWrap> > impl;
    res.Clear();
    impl.Res = &res;
    impl.Start(true);
    impl.Parse(s.data(), s.size());
    impl.Final();
    //return !impl.Failed(); //screw this, unknown tags/attrs get us here
    return res.QueryUTF8.size();
}

}
}
