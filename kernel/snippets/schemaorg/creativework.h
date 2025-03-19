#pragma once

#include <util/generic/list.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NSchemaOrg {
    class TCreativeWork {
    public:
        TString Type;
        TUtf16String Name;
        TUtf16String Headline;
        TUtf16String Description;
        TUtf16String ArticleBody;
        TList<TUtf16String> Author;
        TList<TUtf16String> Genre;
        TUtf16String VideoDuration;
        TUtf16String VideoThumbUrl;

    public:
        TUtf16String GetGenres(size_t maxAmount) const;
        TUtf16String GetAuthors(size_t maxAmount) const;
    };

} // namespace NSchemaOrg
