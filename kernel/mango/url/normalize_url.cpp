#include "normalize_url.h"
#include "info.h"

#include <contrib/libs/re2/re2/re2.h>

#include <kernel/mango/common/html.h>

#include <library/cpp/html/entity/htmlentity.h>
#include <library/cpp/html/face/propface.h>
#include <library/cpp/html/html5/parse.h>
#include <library/cpp/html/zoneconf/ht_conf.h>
#include <library/cpp/html/zoneconf/attrextractor.h>

#include <util/generic/algorithm.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <util/string/strip.h>
#include <util/string/vector.h>
#include <util/string/split.h>

namespace NMango {
    static bool KeepParam(const TString &param, bool isWhiteList, const TVector<TString> &list, bool keepParamsWithoutValue) {
        size_t eqPos = param.find('=');
        if (eqPos == TString::npos) {
            return keepParamsWithoutValue;
        }
        TString paramName = param.substr(0, eqPos);
        bool isInList = Find(list.begin(), list.end(), paramName) != list.end();
        return isInList == isWhiteList;
    }

    static bool CompareParams(const TString &a, const TString &b) {
        return a.substr(0, a.find('=')) < b.substr(0, b.find('='));
    }

    void FilterCgiParamsByList(THttpURL &url, bool isWhiteList, const TVector<TString> &list, bool keepParamsWithoutValue = true, bool sort = false) {
        TVector<TString> oldParams;
        StringSplitter(TString{url.GetField(THttpURL::FieldQuery)}).Split('&').SkipEmpty().Collect(&oldParams);
        TVector<TString> newParams;
        for (TVector<TString>::iterator it = oldParams.begin(); it != oldParams.end(); ++it) {
            if (KeepParam(*it, isWhiteList, list, keepParamsWithoutValue)) {
                newParams.push_back(*it);
            }
        }
        if (sort) {
            Sort(newParams.begin(), newParams.end(), CompareParams);
        }
        TString newQuery = JoinStrings(newParams, "&");
        if (!newQuery.empty()) {
            url.Set(THttpURL::FieldQuery, newQuery);
        } else {
            url.Reset(THttpURL::FieldQuery);
        }
    }

    static void ReparseUrl(THttpURL &url, const TString &newUrl) {
        TUrlInfo info(NMango::NormalizeUrl(newUrl));
        if (info.IsValid())
            url = info.GetParsed();
    }

    static void ReparsePath(THttpURL &url, const TString &newPath) {
        TString baseUrl = url.PrintS(THttpURL::FlagScheme | THttpURL::FlagHostPort);
        ReparseUrl(url, baseUrl + newPath);
    }

    static void ProcessVkontakte(THttpURL &url) {
        TCgiParameters params;
        params.Scan(TString{url.GetField(THttpURL::FieldQuery)});
        const TString &redirect = params.Get("to");
        if (!redirect.empty()) {
            ReparseUrl(url, redirect);
        }
    }

    static void ProcessT30P(THttpURL &url) {
        TString query = TString{url.GetField(THttpURL::FieldQuery)};
        if (!query.empty()) {
            CGIUnescape(query);
            ReparseUrl(url, "http://" + query);
        }
    }

    static void ProcessTwitter(THttpURL &url) {
        TCgiParameters params;
        params.Scan(TString{url.GetField(THttpURL::FieldQuery)});
        TCgiParameters::const_iterator it = params.Find("_escaped_fragment_");
        if (it != params.end() && it->second.StartsWith('/')) {
            ReparsePath(url, it->second);
        }
    }

    static void ProcessLivejournal(THttpURL &url) {
        static const TVector<TString> whitelist = {"thread", "page"};
        FilterCgiParamsByList(url, true, whitelist);
    }

    static TString ExtractActualYoutubePath(const THttpURL &url) {
        TCgiParameters params;
        params.Scan(TString{url.GetField(THttpURL::FieldQuery)});
        TString result = params.Get("next_url");
        if (result.empty()) {
            result = params.Get("next");
        }
        return result;
    }

    static bool ReparseDesktopUri(THttpURL &url) {
        TString result;
        TString fragment = TString{url.GetField(THttpURL::FieldFrag)};
        size_t questionPos = fragment.find('?');
        if (questionPos != TString::npos) {
            TCgiParameters fragmentParams;
            fragmentParams.Scan(fragment.substr(questionPos + 1));
            TString desktopUri = fragmentParams.Get("desktop_uri");
            if (!desktopUri.empty()) {
                CGIUnescape(desktopUri);
                if (!desktopUri.StartsWith("http://")) {
                    desktopUri.prepend("http://www.youtube.com/");
                }
                ReparseUrl(url, desktopUri);
                return true;
            }
        }
        return false;
    }

    static void ProcessYoutube(THttpURL &url) {
        TString path = TString{url.GetField(THttpURL::FieldPath)};
        TString host = TString{url.GetField(THttpURL::FieldHost)};
        bool isMobile = host == "m.youtube.com";
        if (isMobile && ReparseDesktopUri(url)) {
            return;
        }
        if ((path == "/verify_controversy") || (path == "/verify_age") || (path == "/das_captcha")) {
            TString actualPath = ExtractActualYoutubePath(url);
            if (!actualPath.empty()) {
                CGIUnescape(actualPath);
                ReparsePath(url, actualPath);
            }
        }

        path = TString{url.GetField(THttpURL::FieldPath)};
        if (path == "/watch") {
            static const TVector<TString> v = {"v"};
            FilterCgiParamsByList(url, true, v, false);
            if (isMobile) {
                url.Set(THttpURL::FieldHost, "www.youtube.com");
            }
        }
        if (host == "youtube.com") {
            url.Set(THttpURL::FieldHost, "www.youtube.com");
        }
    }

    static void ProcessShortYoutube(THttpURL& url) {
        TString path = TString{url.GetField(THttpURL::FieldPath)};
        if (path.length() > 1) {
            TString videoId = path.substr(1);
            ReparseUrl(url, TString::Join("http://www.youtube.com/watch?v=", videoId));
        }
    }

    static void ProcessHeadlinesRu(THttpURL &url) {
        TString query = TString{url.GetField(THttpURL::FieldQuery)};
        size_t ampPos = query.find('&');
        if (ampPos != TString::npos) {
            query.erase(ampPos);
        }
        ReparseUrl(url, query);
    }

    static void ProcessFacebook(THttpURL &url) {
        if (TString{url.GetField(THttpURL::FieldPath)} == "/l.php") {
            TCgiParameters params;
            params.Scan(TString{url.GetField(THttpURL::FieldQuery)});
            const TString &targetURL = params.Get("u");
            if (!targetURL.empty()) {
                ReparseUrl(url, targetURL);
            }
        }
    }

    static void CutTrailingGarbageOfTinyLink(THttpURL &url) {
        const TStringBuf &path = url.GetField(THttpURL::FieldPath);
        url.Reset(THttpURL::FieldQuery);
        url.Set(THttpURL::FieldPath, path.Head(path.find_first_of("():!,%")));
    }

    TString ReplaceQueryEntities(const TString &url) {
        if (url.find(';') == TString::npos)
            return url;
        TVector<char> buffer(url.length() + 1);
        size_t length;
        if (!HtLinkDecode(url.c_str(), buffer.data(), buffer.size(), length, CODES_UTF8))
            return url;
        return TString(buffer.data(), length);
    }

    TString NormalizeUrl(const TString &rawUrl, const TString& replUrl) {
        TUrlInfo info(ReplaceQueryEntities(Strip(rawUrl)));
        if (!info.IsValid()) {
            return replUrl;
        }
        THttpURL url = info.GetParsed();

        if (info.IsTiny() && !info.IsUrlMangler()) {
            CutTrailingGarbageOfTinyLink(url);
        }

        TString host = TString{url.GetField(THttpURL::FieldHost)};
        TString path = TString{url.GetField(THttpURL::FieldPath)};
        if (((host == "headlines.ru") || (host == "www.headlines.ru") && ((path == "/go.php") || (path == "/gorss.php")))) {
            ProcessHeadlinesRu(url);
        }
        else if (((host == "vkontakte.ru") || (host == "vk.com")) && (path == "/away.php")) {
            ProcessVkontakte(url);
        }
        else if ((host == "t30p.ru") && (path == "/l.aspx")) {
            ProcessT30P(url);
        }
        else if (host == "twitter.com") {
            ProcessTwitter(url);
        } else if (host == "facebook.com" || host == "www.facebook.com") {
            ProcessFacebook(url);
        } else {
            static RE2 postPath("/\\d+\\.html");
            Y_ASSERT(postPath.ok());
            if ((host.EndsWith("livejournal.com")) && (RE2::FullMatch(path.data(), postPath))) {
                ProcessLivejournal(url);
            }
            else if ((host.EndsWith("youtube.com"))) {
                ProcessYoutube(url);
            }
            else if (host == "youtu.be") {
                ProcessShortYoutube(url);
            }
        }

        static const TVector<TString> googleAnalytics = {
            "utm_medium",
            "utm_source",
            "utm_campaign",
            "utm_content",
            "utm_term",
            "spref" // spref=tw in blogspot records
            };
        FilterCgiParamsByList(url, false, googleAnalytics, true, true);

        url.Reset(THttpURL::FieldFrag);
        return TUrlInfo(url).GetRaw();
    }

    class TNormalizer : public IParserResult {
    public:
        TNormalizer(TVector<TString>& urls, IParsedDocProperties* docProps, TAttributeExtractor* ae)
            : Urls(urls)
            , DocProps(docProps)
            , Ae(ae)
        {
            Urls.clear();
        }
        THtmlChunk* OnHtmlChunk(const THtmlChunk& chunk) override {
            TZoneEntry zone;
            Ae->CheckEvent(chunk, DocProps, &zone);
            for (size_t i = 0; i < zone.Attrs.size(); ++i) {
                 const TAttrEntry& attr = zone.Attrs[i];
                 if (attr.Type == ATTR_URL && strcmp(~attr.Name, "link") == 0 && ~attr.Value != nullptr)
                     Urls.push_back(attr.Value);
            }
            return nullptr;
        }
    private:
        TVector<TString>& Urls;
        IParsedDocProperties* DocProps;
        TAttributeExtractor* Ae;
    };

    void NormalizePureTextLinks(TVector<TString>& urls) {
        TString content;
        for (size_t i = 0; i < urls.size(); ++i) {
            if (urls[i].find('"') == TString::npos)
                content += "<a href=\"" + urls[i] + "\"></a>";
        }

        THtConfigurator configurator;
        configurator.LoadDefaultConf();
        THolder<IParsedDocProperties> docProps(CreateParsedDocProperties());
        docProps->SetProperty(PP_CHARSET, "utf-8");
        TAttributeExtractor ae(&configurator);
        TBuffer normalizedDoc;
        normalizedDoc.Assign(content.data(), content.size());
        TNormalizer parserResult(urls, docProps.Get(), &ae);
        NHtml5::ParseHtml(normalizedDoc, &parserResult);
    }
}
