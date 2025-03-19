#pragma once

#include <library/cpp/langs/langs.h>
#include <util/generic/list.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NSchemaOrg {
    class TSoftwareApplication {
    public:
        TUtf16String Description;
        TList<TUtf16String> InteractionCount;
        TUtf16String FileSize;
        TList<TUtf16String> OperatingSystem;
        TUtf16String Price;
        TUtf16String PriceCurrency;
        TList<TUtf16String> ApplicationSubCategory;

    public:
        TUtf16String FormatSnip(TStringBuf host, ELanguage lang) const;
        TUtf16String FormatDescription() const;
        TUtf16String FormatPrice(TStringBuf host, ELanguage lang) const;
        TUtf16String FormatUserDownloads(ELanguage lang) const;
        TUtf16String FormatOperatingSystem() const;
        TUtf16String FormatSubCategory(ELanguage lang) const;
        TUtf16String FormatFileSize(TStringBuf host, ELanguage lang) const;
    };
}
