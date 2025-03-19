#pragma once

#include <library/cpp/html/face/propface.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/string.h>

namespace NHtmlXPath {

struct TDocument {
    TString Url;
    const IParsedDocProperties& DocProps;
    ELanguage Language;

    TDocument(const TString& url, const IParsedDocProperties& docProps, const ELanguage language)
        : Url(url)
        , DocProps(docProps)
        , Language(language)
    {}
};

} // namespace NHtmlXPath

