#include <library/cpp/string_utils/url/url.h>

#include <contrib/libs/re2/re2/re2.h>

#include "info.h"
#include "url_canonizer.h"

namespace NMango {
    static TString CutTrailingSlash(const TString& url) {
        if (url.back() == '/') {
            return url.substr(0, url.length() - 1);
        }
        return url;
    }

    static void CutWWWPrefix(THttpURL& url) {
        url.Set(THttpURL::FieldHost, ::CutWWWPrefix(url.GetField(THttpURL::FieldHost)));
    }

    TURLCanonizer::TURLCanonizer(bool withoutWWW, bool withoutScheme, bool dontStripSuffix)
        : WithoutWWW(withoutWWW)
        , WithoutScheme(withoutScheme)
        , DontStripSuffix(dontStripSuffix)
        // (?i) - case-insensitive matching
        , UselessDocSuffixRegExp(new RE2("(?i)/((index\\.(html|htm|php|pl|jsp|phtml|asp))|(default\\.(asp|aspx)))$"))
    {}

    TURLCanonizer::~TURLCanonizer() {}

    TString TURLCanonizer::Canonize(const TString &url) const {
        TUrlInfo info(url);
        if (!info.IsValid())
            return url;
        THttpURL httpUrl = info.GetParsed();
        if (!DontStripSuffix)
            CutUselessDocument(httpUrl);
        if (WithoutWWW) {
            NMango::CutWWWPrefix(httpUrl);
        }

        if (!WithoutScheme && httpUrl.GetField(THttpURL::FieldScheme) == "https")
            httpUrl.Set(THttpURL::FieldScheme, "http");

        return CutTrailingSlash(TUrlInfo(httpUrl).GetRaw());
    }

    void TURLCanonizer::CutUselessDocument(THttpURL& url) const {
        // важно посмотреть на параметры
        if (url.GetField(THttpURL::FieldQuery).empty() && url.GetField(THttpURL::FieldFragment).empty()
                        && RE2::PartialMatch(url.GetField(THttpURL::FieldPath).data(), *UselessDocSuffixRegExp)) {
            const TStringBuf &path = url.GetField(THttpURL::FieldPath);
            if (!path.empty()) {
                std::string strPath = path.data();
                RE2::Replace(&strPath, *UselessDocSuffixRegExp, "");
                url.Set(THttpURL::FieldPath, strPath.c_str());
            }
        }
    }
}
