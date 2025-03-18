#pragma once

#include <antirobot/lib/ar_utils.h>

#include <util/generic/algorithm.h>
#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>
#include <util/string/vector.h>
#include <util/system/defaults.h>

#include <array>

namespace NAntiRobot {
    constexpr std::array CgiParamName{
        "text"_sb,
        "lr"_sb,
        "rpt"_sb,
        "ed"_sb,
        "clid"_sb,
        "p"_sb,
        "right"_sb,
        "requestid"_sb,
        "left"_sb,
        "spsite"_sb,
        "img_url"_sb,
        "stpar2"_sb,
        "stpar4"_sb,
        "stpar1"_sb,
        "from"_sb,
        "stype"_sb,
        "grhow"_sb,
        "period"_sb,
        "stpar3"_sb,
        "tld"_sb,
        "numdoc"_sb,
        "rstr"_sb,
        "nl"_sb,
        "ncrnd"_sb,
        "url"_sb,
        "dtype"_sb,
        "pid"_sb,
        "cid"_sb,
        "site"_sb,
        "nocookiesupport"_sb,
        "cl4url"_sb,
        "s"_sb,
        "yasoft"_sb,
        "lang"_sb,
        "mime"_sb,
        "within"_sb,
        "from_month"_sb,
        "to_day"_sb,
        "from_year"_sb,
        "from_day"_sb,
        "to_year"_sb,
        "to_month"_sb,
        "rdrnd"_sb,
        "password"_sb,
        "wordforms"_sb,
        "serverurl"_sb,
        "zone"_sb,
        "query"_sb,
        "sp"_sb,
        "rnd"_sb,
        "isize"_sb,
        "msp"_sb,
        "vertical"_sb,
        "icolor"_sb,
        "path"_sb,
        "rd"_sb,
        "geo"_sb,
        "xml"_sb,
        "max-title-length"_sb,
        "maxpassages"_sb,
        "type"_sb,
        "isgray"_sb,
        "ras"_sb,
        "cat"_sb,
        "surl"_sb,
        "iserverurl"_sb,
    };

    constexpr std::array CacherCgiParamName{
        "text"_sb,
        "lr"_sb,
        "clid"_sb,
        "tld"_sb,
        "url"_sb,
        "site"_sb,
        "lang"_sb,
    };

    template<typename T>
    constexpr size_t FindArrayIndex(const T& cgiParamArray, const TStringBuf& paramName) {
        const auto it = Find(cgiParamArray, paramName);
        if (it == cgiParamArray.end()) {
            ythrow yexception() << "unknown key: " << paramName << "\n"
                "in array " << JoinStrings(std::begin(cgiParamArray), std::end(cgiParamArray), " "_sb);
        }
        return it - cgiParamArray.begin();
    }

    /* indexes of CgiParamName array - keep it synced */
    const size_t CGI_TEXT_INDEX = 0;
    const size_t CGI_LR_INDEX = 1;
} // namespace NAntiRobot
