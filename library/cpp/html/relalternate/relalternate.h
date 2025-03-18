#pragma once

#include <util/generic/buffer.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/string/split.h>
#include <util/string/vector.h>

namespace NRelAlternate {
    struct THrefLang {
        TStringBuf Href;
        TStringBuf Lang;
        THrefLang(TStringBuf href, TStringBuf lang)
            : Href(href)
            , Lang(lang)
        {
        }
        inline bool operator==(const THrefLang& r) const {
            return (Href == r.Href) && (Lang == r.Lang);
        }
    };
    using THrefLangVector = TVector<THrefLang>;

    TString ConcatenateLangHref(const TStringBuf& lang, const TStringBuf& href);
    void SplitLangHref(const TStringBuf& langHref, TStringBuf& lang, TStringBuf& href);
    template <class T>
    void AppendRelAlternate(T& bufferToAppend, TString const& langHref);
    TVector<TStringBuf> SplitRelAlternate(const TStringBuf& relAlternate);

    THrefLangVector SplitAlternateHreflang(const TStringBuf& relAlternateLinks);

}

#include "relalternate-inl.h"
