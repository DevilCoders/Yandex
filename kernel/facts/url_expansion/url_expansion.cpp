#include "url_expansion.h"

#include <quality/functionality/turbo/urls_lib/cpp/lib/turbo_urls.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/string_utils/url/url.h>

#include <util/charset/utf8.h>
#include <util/string/split.h>

namespace NFacts {

    TString ExpandReferringUrl(const TString& url) {
        // first check for Yandex Turbo URL
        if (NTurbo::IsTurboUrl(url, true)) {
            return NTurbo::GetOriginalUrl(url);
        }

        TStringBuf splitUrl, query, fragment;
        SeparateUrlFromQueryAndFragment(TStringBuf(url), splitUrl, query, fragment);
        TStringBuf host, path;
        SplitUrlToHostAndPath(splitUrl, host, path);
        TCgiParameters params(query);

        TVector<TString> domains;
        const TString hostNormalized = TString(CutSchemePrefix(ToLowerUTF8(host)));
        StringSplitter(hostNormalized.data(), hostNormalized.end()).Split('.').AddTo(&domains);
        // now check for a Yandex Translate URL
        if (domains.size() > 2 && domains[0] == "translate" && domains[1] == "yandex") {
            const TString& urlText = params.Get("url"); // TCgiParameters have decoded the URL already
            if (!urlText.empty()) {
                return urlText;
            }
        }
        return url;
    }

}
