#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/maybe.h>

namespace NSnippets {

struct TQueryResultDoc {
    TString Source;
    TString Url;
    TString SnippetType;

    TQueryResultDoc() = default;

    TQueryResultDoc(const TString& source, const TString& url, const TString& type)
        : Source(source)
        , Url(url)
        , SnippetType(type)
    {
    }
};

    class TQueryLog {
    public:
        TString Query;
        TString FullRequest;
        TMaybe<TString> CorrectedQuery;
        TString UserRegion;
        TString DomRegion;
        TString UILanguage;
        TString RequestId;
        TMaybe<TString> SnipWidth;
        TMaybe<TString> ReportType;
        TVector<TQueryResultDoc> Docs;
        TMaybe<TString> MRData;

        TQueryLog() = default;
        TString ToString() const;
        static TQueryLog FromString(const TString& str);
    };

}
