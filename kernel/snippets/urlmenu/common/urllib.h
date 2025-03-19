#pragma once

#include <library/cpp/uri/http_url.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/string_utils/url/url.h>

namespace NSitelinks {
    static inline TString NormalizeUrl(const TString& url) {
        TString normalizedUrl(url);
        UrlUnescape(normalizedUrl);
        normalizedUrl = TString("http://") + CutHttpPrefix(normalizedUrl);
        THttpURL parsedUrl;
        THttpURL::TParsedState parseResult = parsedUrl.Parse(normalizedUrl);
        if (THttpURL::ParsedOK != parseResult)
            return "";

        TString netloc(parsedUrl.GetField(THttpURL::FieldHost));
        netloc.to_lower();

        TString tail(parsedUrl.GetField(THttpURL::FieldPath));
        UrlEscape(tail);

        TString query(parsedUrl.GetField(THttpURL::FieldQuery));
        if (!query.empty()) {
            TCgiParameters params(query);

            if (params.size() == 0) {
                CGIEscape(query);
                tail += "?" + query;
            } else {
                tail += '?';
                tail += params.Print();
            }
        }

        TString fragment(parsedUrl.GetField(THttpURL::FieldFragment));
        if (! fragment.empty())
            tail += "#" + fragment;

        return netloc + tail;
    }

    static inline void ListPageParents(const TString& url, TVector<TString>& parents) {
        TString normalizedUrl = NSitelinks::NormalizeUrl(url);
        parents.clear();
        for (size_t rbound = normalizedUrl.find_first_of("?#");;) {
            if (rbound < normalizedUrl.size() - 1) {
                if (normalizedUrl[rbound] != '/')
                    --rbound;
                parents.push_back(normalizedUrl.substr(0, rbound + 1));
            }
            rbound = normalizedUrl.find_last_of("/", rbound - 1, 1);
            if (rbound == TString::npos)
                break;
        }
    }
};
