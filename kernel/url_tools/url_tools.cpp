#include "url_tools.h"

#include <library/cpp/tld/tld.h>
#include <library/cpp/uri/uri.h>
#include <util/generic/strbuf.h>
#include <util/string/strip.h>
#include <library/cpp/string_utils/url/url.h>

TIsUrlResult::TIsUrlResult()
    : IsUrl(false)
    , IsIDNA(false)
{
}

TIsUrlResult::TIsUrlResult(bool isUrl, const TString& link, const TString& encoded, bool isIDNA, const TString& idnaLink)
    : IsUrl(isUrl)
    , ResUrl(link)
    , ResUrlEncoded(encoded)
    , IsIDNA(isIDNA)
    , ResUrlIDNA(idnaLink)
{
}

inline bool FastCheck(const TStringBuf& req) {
    return req.find('.') != TStringBuf::npos;
}

inline void Strip(TStringBuf& req) {
    StripString(req, req);
    if (req.StartsWith("\"") && req.EndsWith("\""))
        req.Skip(1).Chop(1);
    if (req.StartsWith("\\\"") && req.EndsWith("\\\""))
        req.Skip(2).Chop(2);
}

inline bool IsIDNA(TStringBuf host) {
    constexpr TStringBuf acePrefix = "xn--";

    size_t pos;
    while (host.size()) {
        if (host.StartsWith(acePrefix))
            return true;
        pos = host.find('.');
        if (pos == TStringBuf::npos)
            break;
        host.Skip(pos + 1);
    }
    return false;
}

TIsUrlResult IsUrl(TStringBuf req, int flags) {
    enum {
        PARSER_FLAGS = ::NUri::TUri::FeatureSchemeFlexible | ::NUri::TUri::FeatureToLower
    };
    TIsUrlResult res;

    do {
        if (!FastCheck(req))
            break;

        Strip(req);

        ::NUri::TUri reguri;
        if (::NUri::TUri::ParsedOK != reguri.ParseAbsOrHttpUri(req, PARSER_FLAGS | ::NUri::TUri::FeatureAllowHostIDN | ::NUri::TUri::FeatureCheckHost))
            break;

        const TStringBuf& host = reguri.GetHost();
        if (!NTld::InTld(host))
            break;

        // registration of IDN in .ru is not allowed
        const bool isIDNA = IsIDNA(host);
        if (isIDNA && GetZone(host) == "ru")
            break;

        ::NUri::TUri encuri;
        // this can't fail if previous parse didn't
        encuri.ParseAbsOrHttpUri(req, PARSER_FLAGS | ::NUri::TUri::FeaturesEncodeDecode);
        // use the ascii host
        encuri.FldMemUse(::NUri::TUri::FieldHost, host);

        res.IsUrl = true;
        res.IsIDNA = isIDNA;

        reguri.Print(res.ResUrl, flags);
        if (reguri.GetFieldMask() & ::NUri::TUri::FlagHostAscii)
            reguri.Print(res.ResUrlIDNA, flags | ::NUri::TUri::FlagHostAscii);
        else
            res.ResUrlIDNA = res.ResUrl;
        encuri.Print(res.ResUrlEncoded, flags);
    } while (false);

    return res;
}
