#include <util/charset/utf8.h>
#include <library/cpp/charset/recyr.hh>
#include <library/cpp/charset/wide.h>
#include <util/string/subst.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/string_utils/url/url.h>

#include <kernel/url_tools/idna_decode.h>
#include <kernel/keyinv/invkeypos/keyconv.h>

#include "url_variants.h"

void TUrlVariants::EnhanceWithSlashesAndWWW(const TString& src) {
    static constexpr TStringBuf www = "www.";
    Slashify(src);
    if (src.size() > www.size() && src.StartsWith(www)) {
        Slashify(src.substr(www.size()));
    } else {
        Slashify(www + src);
    }
}

void TUrlVariants::AddWithLowerCased(const TString& src) {
    Urls.insert(src);
    TString lower = src;
//https://st.yandex-team.ru/SEARCH-646
//http://clubs.at.yandex-team.ru/search-quality/671
    if (UTF8Detect(src) == UTF8) {
        TString utf8lower = ToLowerUTF8(src);
        Urls.insert(utf8lower);
        TUtf16String wideSrc = UTF8ToWide(src);
        TString cp1251Src = WideToChar(wideSrc.data(), wideSrc.size(), CODES_WIN);
        Urls.insert(cp1251Src);
        TString lower1251 = ToLower(cp1251Src, csYandex);
        if (lower1251 != cp1251Src) {
            Urls.insert(lower1251);
        }
        //special style of csYandex attributes encoding support
        //todo: delete during migration to utf8 only attributes
        TFormToKeyConvertor attribConvertor;
        Urls.insert(attribConvertor.ConvertAttrValue(wideSrc.data(), wideSrc.size()));
    }
    else {
        if (lower.to_lower()) {
            Urls.insert(lower);
        }
        TUtf16String wideSrc = CharToWide(src, CODES_WIN);
        wideSrc.to_lower();
        TFormToKeyConvertor attribConvertor;
        Urls.insert(attribConvertor.ConvertAttrValue(wideSrc.data(), wideSrc.size()));
    }
}

void  TUrlVariants::Slashify(const TString& src) {
    const char slash = '/';  // bug was here (backslash)
    AddWithLowerCased(src);
    if (src.size() > 1 && src.back() == slash) {
        AddWithLowerCased(src.substr(0, src.size() - 1));
    } else {
        AddWithLowerCased(src + slash);
    }
}

void TUrlVariants::ProcessUrl(TString src) {
    src.to_lower();
    src = CutWWWPrefix(TString{CutHttpPrefix(src)});
    EnhanceWithSlashesAndWWW(src);
}

void TUrlVariants::URLUnescapeAndProcess(const TString& src) {
    EnhanceWithSlashesAndWWW(src);
    TString unescapedUrl(src);
    // some url keys in base are encoded with everything except spaces unescaped
    // so we don't touch '+' in url and if we decoded %20 as space we encode it back
    UrlUnescape(unescapedUrl);
    if (src != unescapedUrl) {
        EnhanceWithSlashesAndWWW(unescapedUrl);
    }
    SubstGlobal(unescapedUrl, " ", "%20");
    EnhanceWithSlashesAndWWW(unescapedUrl);
    SubstGlobal(unescapedUrl, "+", "%20");
    EnhanceWithSlashesAndWWW(unescapedUrl);
}

void TUrlVariants::IDNAProcess(const TString& src) {
    URLUnescapeAndProcess(src);
    TString idnaDecoded;
    if (IDNAUrlToUtf8(src, idnaDecoded, true)) {
        URLUnescapeAndProcess(idnaDecoded);
        URLUnescapeAndProcess(Recode(CODES_UTF8, CODES_WIN, idnaDecoded));
    }
}

