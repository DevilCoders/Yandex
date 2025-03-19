#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/system/maxlen.h>
#include <util/stream/output.h>
#include <util/string/cast.h>

/// Specification:
/// https://developers.google.com/webmasters/ajax-crawling/docs/specification

/// In few words, this function does follow conversions:
/// * '#!' -> '?_escaped_fragment_='
/// * Lowercase for host
/// * Host to punycode
/// * Percent-encoding for path
/// * Fragments (part after #, '#smth') will be cutted
/// * If path is empty, '/' will be added to host ('http://ya.ru' -> 'http://ya.ru/')
/// * If scheme doesn't exist, you MUST add it yourself ('ya.ru/' != 'http://ya.ru/')
/// * If was an error when normalizing, will be returned incoming url
/// See examples at kernel/urlnorm/normalize_ut.h
/// Note: url is expected to be in ASCII or Utf8.
TString DoWeakUrlNormalization(const TStringBuf& incomingUrl, bool* result = nullptr);

namespace NPrivate {

Y_FORCE_INLINE
bool TryUrlToRobotUrl_DONT_USE_THIS_FUNCTION(const TStringBuf& rawUrl, TString& result, bool requireGlobal) noexcept
{
    const size_t schemeSize = GetSchemePrefixSize(rawUrl);

    if (schemeSize == 0 && requireGlobal) {
        return false;
    }

    bool state;
    TString url;
    if (schemeSize == 0) {
        url = DoWeakUrlNormalization("http://" + ToString(rawUrl), &state);
    } else {
        url = DoWeakUrlNormalization(rawUrl, &state);
    }

    if (!state) {
        return false;
    }

    if (url.StartsWith("http://")) {
        url = url.substr(7);
    }

    result = url;
    return true;
}

Y_FORCE_INLINE
bool UrlToRobotUrl_DONT_USE_THIS_FUNCTION(const TStringBuf& rawUrl, TString& result, bool requireGlobal) noexcept
{
    if (TryUrlToRobotUrl_DONT_USE_THIS_FUNCTION(rawUrl, result, requireGlobal)) {
        return true;
    } else {
        result.clear();
        return false;
    }
}

} // NPrivate


/// Converts url to proper robot url:
/// *    - removes http schema, but keeps https
/// *    - removes fragment
/// *    - removes default port
/// *    - lowercases host
/// *    - punycodes host if needed
/// *    - encodes #!
/// *    - prepare url for UrlHashVal
/// * @note if failed to parse, result will not be affected.
/// * @return true if url is parsed, false otherwise.
/// * @param requireGlobal - if set to true, requires src url to be global (contains schema).
// Don't use this function
// Use NUrlNorm::NormalizeUrl function instead
Y_FORCE_INLINE
bool TryUrlToRobotUrl_DONT_USE_THIS_FUNCTION(const TStringBuf& rawUrl, TString& result, bool requireGlobal) noexcept
{
    return NPrivate::TryUrlToRobotUrl_DONT_USE_THIS_FUNCTION(rawUrl, result, requireGlobal);
}

struct TUrlToRobotUrlOptions {
    bool    RequireGlobal;
    size_t  MaxUrlLength;

    TUrlToRobotUrlOptions()
        : RequireGlobal(false)
        , MaxUrlLength(URL_MAX)
    {
    }
};

/// Same as TryUrlToRobotUrl_DONT_USE_THIS_FUNCTION, but result will be cleared if failed
// Don't use this function
// Use NUrlNorm::NormalizeUrl function instead
Y_FORCE_INLINE
bool UrlToRobotUrl_DONT_USE_THIS_FUNCTION(const TStringBuf& rawUrl, TString& result, bool requireGlobal) noexcept
{
    return NPrivate::UrlToRobotUrl_DONT_USE_THIS_FUNCTION(rawUrl, result, requireGlobal);
}

// Don't use this function
// Use NUrlNorm::NormalizeUrl function instead
Y_FORCE_INLINE
bool UrlToRobotUrl_DONT_USE_THIS_FUNCTION(const TUrlToRobotUrlOptions& opts, const TStringBuf& rawUrl, TString& result) noexcept
{
    if (NPrivate::UrlToRobotUrl_DONT_USE_THIS_FUNCTION(rawUrl, result, opts.RequireGlobal) && result.length() < opts.MaxUrlLength) {
        return true;
    }

    return false;
}

namespace NUrlNorm {

static const TString DEFAULT_SCHEME = "http://";

/// Converts url to proper Content System (aka Robot) url:
/// *    - removes fragment
/// *    - removes default port
/// *    - lowercases host
/// *    - punycodes host if needed
/// *    - encodes #!
/// *    - for empty path adds default path ("/")
/// * @note if failed to parse, result will not be affected.
/// * @return true if url is parsed, false otherwise.
bool NormalizeUrl(const TStringBuf url, TString& result);

/// Converts url to proper Content System (aka Robot) url:
/// *    - removes fragment
/// *    - removes default port
/// *    - lowercases host
/// *    - punycodes host if needed
/// *    - encodes #!
/// *    - for empty path adds default path ("/")
/// * @return normilized url if url is parsed, throws yexception otherwise.
TString NormalizeUrl(const TStringBuf url);

/// Converts url from normalized with NormalizeUrl to walrus-friendly form:
// *     - removes http:// scheme prefix
// *     - drops last slash
// * @note does not checks if url was really normalized with NormalizeUrl
TString DenormalizeUrlForWalrus(TStringBuf url);

/// Will remove UTMs and popular redirectors.
/// This normalization type if not canonical. Use it on your own risk!
TString NormalizeDirectUrlDraft(const TString& src);

/// Removes all cgi parameters which match a given predicate from url
TString NormalizeUrlCgi(const TStringBuf url, std::function<bool(const TString&)> pred);

/// Checks whether a given cgi param is utm or yrwinfo
bool IsUtmOrYrwinfo(const TString& cgiParam);

/// Removes cgi and fragment from url
TString RemoveCgi(const TStringBuf url);

} // NUrlNorm

