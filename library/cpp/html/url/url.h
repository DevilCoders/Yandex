#pragma once

#include <library/cpp/charset/doccodes.h>
#include <library/cpp/uri/uri.h>

namespace NHtml {
    ::NUri::TUri::TLinkType NormalizeLink(const ::NUri::TUri& base, TStringBuf link, ::NUri::TUri* result, ECharset idnaEnc, ECharset decodeEnc);

    inline ::NUri::TUri::TLinkType NormalizeLink(const ::NUri::TUri& base, const char* link, ::NUri::TUri* result, ECharset idnaEnc, ECharset decodeEnc) {
        return NormalizeLink(base, TStringBuf(link, strlen(link)), result, idnaEnc, decodeEnc);
    }

    /// parse link from web page and convert it to form suitable for crawling
    /// use encoding for host idna aka punycode only, byte-encode local chars
    inline ::NUri::TUri::TLinkType NormalizeLinkForCrawl(const ::NUri::TUri& base, const char* link, ::NUri::TUri* result, ECharset idnaEnc) {
        return NormalizeLink(base, link, result, idnaEnc, CODES_UNKNOWN);
    }
    inline ::NUri::TUri::TLinkType NormalizeLinkForCrawl(const ::NUri::TUri& base, TStringBuf link, ::NUri::TUri* result, ECharset idnaEnc) {
        return NormalizeLink(base, link, result, idnaEnc, CODES_UNKNOWN);
    }

    /// parse link from web page and convert it to form suitable for indexing words
    /// convert local characters to %-encoded utf-8
    inline ::NUri::TUri::TLinkType NormalizeLinkForSearch(const ::NUri::TUri& base, const char* link, ::NUri::TUri* result, ECharset enc) {
        return NormalizeLink(base, link, result, enc, enc);
    }
    inline ::NUri::TUri::TLinkType NormalizeLinkForSearch(const ::NUri::TUri& base, TStringBuf link, ::NUri::TUri* result, ECharset enc) {
        return NormalizeLink(base, link, result, enc, enc);
    }

}
