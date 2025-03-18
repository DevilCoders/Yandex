#pragma once

#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <library/cpp/string_utils/quote/quote.h>

namespace NSnippets {
namespace NXmlSearchIn {

struct TDocument {
    TString HilitedUrl;
    TString SerializedUrlmenu;
    TVector<TString> SearchHttpUrl;
};

struct TRequest {
    TString QueryUTF8;
    TString FullQuery;
    TString Qtree;
    TVector<TDocument> Documents;
    void Clear() {
        QueryUTF8.clear();
        FullQuery.clear();
        Qtree.clear();
        Documents.clear();
    }
};

bool ParseRequest(TRequest& res, TStringBuf s);

}
}
