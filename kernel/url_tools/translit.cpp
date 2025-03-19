#include "translit.h"

#include "url_tools.h"

#include <util/string/util.h>
#include <util/charset/wide.h>
#include <library/cpp/charset/recyr.hh>
#include <util/charset/utf8.h>
#include <library/cpp/uri/http_url.h>

#include <contrib/libs/libidn/lib/idna.h>

#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/untranslit/untranslit.h>

/*********** Translit ***********/

inline bool IsUTF8(char c) {
#define IUC(X) ((int)(unsigned char)(X))
    return IUC(c) & 0x80;
#undef IUC
}

static TString PurifyToken(const TString& token) {
    TString result;
    for (size_t i = 0; i < token.size(); ++i)
        if (token[i] != '\'')
            result += token[i];
    return result;
}

static TString TranslitToken(const TString& token, TUntransliter* transliter) {
    constexpr TStringBuf ru = "ру";
    if (token == ru)
        return "ru";
    constexpr TStringBuf com = "ком";
    if (token == com)
        return "com";
    constexpr TStringBuf ua = "уа";
    if (token == ua)
        return "ua";
    constexpr TStringBuf net = "нет";
    if (token == net)
        return "net";
    constexpr TStringBuf net2 = "нэт";
    if (token == net2)
        return "net";
    transliter->Init(UTF8ToWide(token));
    TUntransliter::WordPart wp = transliter->GetNextAnswer();
    if (!wp.Empty())
        return PurifyToken(WideToUTF8(wp.GetWord()));
    else
        return token;
}

static bool Transliterate(const TString& url, TString* res) {
    static const size_t MAX_TOKENS = 5;

    const size_t urlLen = url.size();
    size_t nTokens = 0;
    bool state = false;
    for (size_t i = 0; i <= urlLen; ++i) {
        bool isUTF8 = IsUTF8(url[i]);
        if (state) {
            if (!isUTF8) {
                ++nTokens;
                if (nTokens > MAX_TOKENS)
                    return false;
                state = false;
            }
        } else {
            if (isUTF8)
                state = true;
        }
    }
    if (0 == nTokens)
        return false;

    TAutoPtr<TUntransliter> transliter = NLemmer::GetLanguageById(LANG_RUS)->GetTransliter();
    TString result;
    TString buffer;
    for (size_t i = 0; i <= urlLen; ++i) {
        if (IsUTF8(url[i])) {
            buffer += url[i];
        } else {
            if (buffer.size()) {
                result += TranslitToken(buffer, transliter.Get());
                buffer.clear();
            }
            if (i != urlLen)
                result += url[i];
        }
    }
    *res = result;
    return true;
}

TIsUrlSmartTransliterateResult::TIsUrlSmartTransliterateResult()
    : IsExact(false)
    , IsTranslit(false)
{
}

TIsUrlSmartTransliterateResult& TIsUrlSmartTransliterateResult::operator=(const TIsUrlSmartResult& res) {
    IsUrl = res.IsUrl;
    ResUrl = res.ResUrl;
    ResUrlEncoded = res.ResUrlEncoded;
    ResUrlIDNA = res.ResUrlIDNA;
    IsStraight = res.IsStraight;
    IsIDNA = res.IsIDNA;
    IsTranslit = false;
    IsExact = true;
    return *this;
}

TIsUrlSmartTransliterateResult IsUrlSmartTranslit(const TString& req, int flags, bool tryNoTranslit) {
    TIsUrlSmartTransliterateResult result;

    bool isAscii = (UTF8Detect(req.data(), req.size()) == ASCII);

    if (isAscii || tryNoTranslit) {
        result = IsUrlSmart(req, flags);
    }
    if (!result.IsUrl) {
        try {
            if (Transliterate(req, &result.Translit)) {
                if (req != result.Translit) {
                    result = IsUrlSmart(result.Translit, flags);
                    if (result.IsUrl) {
                        result.IsTranslit = true;
                        result.IsExact = false;
                    }
                }
            }
        } catch (...) {
            return result;
        }
    }

    return result;
}
