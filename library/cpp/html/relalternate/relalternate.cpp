#include "relalternate.h"
#include <util/string/cast.h>

namespace NRelAlternate {
    TString ConcatenateLangHref(const TStringBuf& lang, const TStringBuf& href) {
        return ToString(lang) + " " + ToString(href);
    }

    void SplitLangHref(const TStringBuf& langHref, TStringBuf& lang, TStringBuf& href) {
        if (!langHref.TrySplit(' ', lang, href)) {
            href = langHref;
        }
    }

    TVector<TStringBuf> SplitRelAlternate(const TStringBuf& relAlternate) {
        // in attr lang goes first, href goes second
        TVector<TStringBuf> langHrefList;
        StringSplitter(relAlternate).Split('\t').SkipEmpty().Collect(&langHrefList);
        return langHrefList;
    }

    // splits attribute data containing e.g.
    // "nl http://www.booking.com/index.nl.html\tde http://www.booking.com/index.de.html\ten-us http://www.booking.com/\ten-gb http://www.booking.com/index.en-gb.html\t"
    THrefLangVector SplitAlternateHreflang(const TStringBuf& relAlternateLinks) {
        THrefLangVector result;
        // in attr lang goes first, href goes second
        for (auto& langHref : SplitRelAlternate(relAlternateLinks)) {
            if (langHref.size() > 0) {
                TStringBuf lang;
                TStringBuf href;
                SplitLangHref(langHref, lang, href);
                result.push_back(THrefLang(href, lang));
            }
        }
        return result;
    }

}
