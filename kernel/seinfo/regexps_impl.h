#pragma once

#include "enums.h"

#include <util/generic/array_ref.h>

namespace NSe
{
    // Use this class when search page can be easy indicated by host and cgi params
    struct TSESimpleRegexp
    {
        // regexp info
        TStringBuf Regexp;
        TStringBuf CgiParam;
        TStringBuf CgiMagic;

        TSESimpleRegexp(const TStringBuf& regexp, const TStringBuf& cgi = "", const TStringBuf& cgiMagic = "")
            : Regexp(regexp)
            , CgiParam(cgi)
            , CgiMagic(cgiMagic)
        {
        }
    };

    // Note: you should use special named groups:
    // "engine" indicates search engine (value is SE name)
    // "query"  indicates search query itself
    // any named ESearchType

    // Gained from https://svn.yandex.ru/websvn/wsvn/conv/trunk/metrica/src/db_dumps/Metrica/SearchEngines_SimplePatterns.sql
    // and         https://svn.yandex.ru/websvn/wsvn/conv/trunk/metrica/src/db_dumps/Metrica/SearchEngines_Patterns.sql
    // and         other sources

    using TSquareRegexRegion = TArrayRef<const TArrayRef<const TSESimpleRegexp>>;

    const TSquareRegexRegion& GetDefaultSearchRegexps();
    const TSquareRegexRegion& GetNotRagelizedRegexps();
};
