#include "normalize.h"

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/string_utils/url/url.h>
#include <library/cpp/uri/uri.h>

#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/stream/str.h>
#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/string/strip.h>
#include <util/string/subst.h>

TString DoWeakUrlNormalization(const TStringBuf& incomingUrl, bool* result) {
    using namespace NUri;
    const TParseFlags flags = TFeature::FeaturesRecommended;

    TUri uri;
    const TState::EParsed parseResult = uri.Parse(incomingUrl, flags);

    if (result) {
        *result = (parseResult == TState::ParsedOK);
    }

    if (parseResult == TState::ParsedOK) {
        TString normalizedUrl;
        uri.Print(normalizedUrl, TField::FlagAllFields ^ TField::FlagFragment);
        return normalizedUrl;
    } else {
        return ToString(incomingUrl);
    }
}

static
bool HasBraces(const TStringBuf& param) {
    const auto pos1 = param.find('{');
    if (pos1 == TStringBuf::npos) {
        return false;
    }
    const auto pos2 = param.find('}');
    return (pos2 != TStringBuf::npos && pos2 > pos1);
}

namespace NUrlNorm {

bool NormalizeUrl(const TStringBuf rawUrl, TString& result) {
    bool appendDefaultScheme = true;
    bool cutScheme = false;

    const size_t schemeSize = GetSchemePrefixSize(rawUrl);

    bool state;
    TString url;

    if (appendDefaultScheme && schemeSize == 0) {
        url = DoWeakUrlNormalization(DEFAULT_SCHEME + ToString(rawUrl.StartsWith("//") ? rawUrl.Tail(2) : rawUrl), &state);
    } else if (!appendDefaultScheme && schemeSize == 0) {
        return false;
    } else {
        url = DoWeakUrlNormalization(rawUrl, &state);
    }

    if (!state) {
        return false;
    }

    if (cutScheme && url.StartsWith(DEFAULT_SCHEME)) {
        url = url.substr(DEFAULT_SCHEME.size());
    }

    result = url;
    return true;
}

TString NormalizeUrl(const TStringBuf rawUrl) {
    TString result;
    Y_ENSURE(NormalizeUrl(rawUrl, result), "Can not parse url: " << TString{rawUrl}.Quote());
    return result;
}

TString DenormalizeUrlForWalrus(TStringBuf url) {
    const size_t schemeSize = GetSchemePrefixSize(url);
    while (!url.empty() && url.back() == '/') {
        url.Trunc(url.length() - 1);
    }
    if (schemeSize && url.StartsWith(DEFAULT_SCHEME))
        url.Skip(schemeSize);
    return ToString(url);
}

static TString ScanCgi(const TStringBuf query, const TStringBuf cgi) {
    TStringBuf sep = query.Contains("&amp;") ? "&amp;" : "&";
    for (auto&& keyValue : StringSplitter(query).SplitByString(sep)) {
        TString key, value;
        try {
            StringSplitter(TStringBuf{keyValue}).Split('=').Limit(2).CollectInto(&key, &value);
            CGIUnescape(key);
            CGIUnescape(value);

            if (key == cgi) {
                return value;
            }
        } catch (const yexception&) {
            // nothing do.
        }

    }
    return TString{};
}

static
bool UrlFromCgi(TString& url, const TStringBuf paramName, bool decodeBase64 = false) {
    const size_t cgiBegin = url.find('?');
    if (cgiBegin != TString::npos) {
        const size_t cgiEnd = url.find('#', cgiBegin + 1);
        const TStringBuf cgiStr =
            (cgiEnd != TString::npos)
            ? TStringBuf(url.data() + cgiBegin + 1, url.data() + cgiEnd)
            : TStringBuf(url.data() + cgiBegin + 1, url.data() + url.size());

        if (const auto cgiValue = ScanCgi(cgiStr, paramName); !cgiValue.empty()) {
            url = cgiValue;
            if (decodeBase64) {
                try {
                    url = Base64Decode(url);
                } catch (yexception& e) {
                    if (!url.StartsWith("http://") && !url.StartsWith("https://")) {
                        return false;
                    }
                }
            }
            StripInPlace(url);
            return true;
        }
    }
    return false;
}

namespace {
    const THashSet<TString> AdsBadCgis = {
        "block",
        "position",
        "source",
        "roistat",
        "roistat_pos",
        "roistat_referrer",
        "added",
        // "keyword", # Too many stupids.
        // "type",
        // "key",
        "pos",
        "cm_id",
        "region_name",
        "region",
        "device",
        "gbid",
        "yclid",
        "gclid",
        "from",
        "rs",
        "position_type",
        "uid",
        "partner_id",
        "cpamit_uid",
        "ad_id",
        "campaign_id",
        "phrase_id",
        "sub_id",
        "show-uid",
        "ad",
        "advertising",
        "campaign",
        "compaign_id",
        "ad_id",
        "yd_ad_id",
        "yd_campaign_id",
    };
};

TString NormalizeDirectUrlDraft(const TString& src) {
    TString result = src;

    // Resolving most common redirects.
    while (true) {
        bool changed = false;

        TStringBuf srcNoPrefix = CutHttpPrefix(result);
        const auto onlyHost = GetOnlyHost(srcNoPrefix);

        if (srcNoPrefix.StartsWith("pixel.everesttech.net/")) {
            const size_t pos = result.find("url=!");
            if (pos != TString::npos) {
                result = result.substr(pos + 5);
                changed = true;
            } else {
                changed = UrlFromCgi(result, "url");
            }
        } else if (srcNoPrefix.StartsWith("alterprice.ru/")) {
            changed = UrlFromCgi(result, "utm_campaign");
        } else if (UrlFromCgi(result, "ulp")) {
            changed = true;
        } else if (srcNoPrefix.StartsWith("ad.atdmt.com/")) {
            changed = UrlFromCgi(result, "h");
        } else if (srcNoPrefix.StartsWith("clckto.ru/")) {
            const size_t pos = result.find("&to=");
            if (pos != TString::npos) {
                result = result.substr(pos + 4);
                changed = true;
            }
        } else if (srcNoPrefix.StartsWith("ad.doubleclick.net/")) {
            changed = UrlFromCgi(result, "ds_dest_url");
            if (!changed) {
                const auto pos = srcNoPrefix.find('?');
                if (pos != TString::npos) {
                    const auto nr = srcNoPrefix.substr(pos + 1);
                    if (nr.StartsWith("http")) {
                        result = nr;
                        changed = true;
                    }
                }
            }
        } else if (srcNoPrefix.SkipPrefix("eda.yandex/lavka/&")) {
            result = TString::Join("https://eda.yandex/lavka/?", srcNoPrefix);
            changed = true;
        } else if (onlyHost.EndsWith(".laredoute.ru")) {
            const auto pos = srcNoPrefix.find("&kaRdt=");
            if (pos != TString::npos) {
                result = srcNoPrefix.substr(pos + 7);
                changed = true;
            }
        } else if (onlyHost.EndsWith(".actionpay.ru")) {
            const auto pos = srcNoPrefix.find("/url=");
            if (pos != TString::npos) {
                try {
                    result = CGIUnescapeRet(srcNoPrefix.substr(pos + 5));
                    changed = true;
                } catch (...) {
                }
            }
        } else if (srcNoPrefix.StartsWith("www.qxplus.ru") || srcNoPrefix.StartsWith("qxplus.ru")) {
            changed = UrlFromCgi(result, "redir");
        } else if (srcNoPrefix.StartsWith("www.agoda.com")) {
            changed = UrlFromCgi(result, "url");
        } else if (srcNoPrefix.StartsWith("track.himba.ru")) {
            changed = UrlFromCgi(result, "url");
        } else if (onlyHost.EndsWith("xg4ken.com")) {
            changed = UrlFromCgi(result, "url[]");
        } else if (onlyHost.EndsWith("mixmarket.biz")) {
            changed = UrlFromCgi(result, "redir");
        } else if (onlyHost == "s.click.aliexpress.com") {
            changed = UrlFromCgi(result, "dl_target_url");
        } else if (onlyHost.EndsWith(".doubleclick.net")) {
            const auto pos1  = srcNoPrefix.find("?http");
            if (pos1 != TStringBuf::npos) {
                result = srcNoPrefix.substr(pos1 + 1, result.size() - pos1 - 1);
                changed = true;
            }
        } else if (onlyHost == "aff.optionbit.com") {
            const auto pos1  = srcNoPrefix.find("TargetURL=http");
            if (pos1 != TStringBuf::npos) {
                result = srcNoPrefix.substr(pos1 + 10, result.size() - pos1 - 10);
                changed = true;
            }
        } else if (onlyHost == "app.appsflyer.com") {
            const auto pos1  = srcNoPrefix.find("af_r=http");
            if (pos1 != TStringBuf::npos) {
                result = srcNoPrefix.substr(pos1 + 5, result.size() - pos1 - 5);
                changed = true;
            }
        } else if (onlyHost == "clickserve.dartsearch.net") {
            const auto pos1  = srcNoPrefix.find("ds_dest_url=http");
            if (pos1 != TStringBuf::npos) {
                result = srcNoPrefix.substr(pos1 + 12, result.size() - pos1 - 12);
                changed = true;
            } else {
                changed = UrlFromCgi(result, "url");
            }
        } else if (onlyHost == "clk.tradedoubler.com") {
            const auto pos1  = srcNoPrefix.find("url=http");
            if (pos1 != TStringBuf::npos) {
                result = srcNoPrefix.substr(pos1 + 4, result.size() - pos1 - 4);
                changed = true;
            }
        } else if (onlyHost == "go.sellaction.net" || onlyHost == "yadcounter.ru") {
            changed = UrlFromCgi(result, "url");
        } else if (onlyHost == "href.li") {
            const auto pos1  = srcNoPrefix.find("?http");
            if (pos1 != TStringBuf::npos) {
                result = srcNoPrefix.substr(pos1 + 1, result.size() - pos1 - 1);
                changed = true;
            }
        } else if (onlyHost == "m.vk.com" || onlyHost == "vk.com") {
            changed = UrlFromCgi(result, "to");
        } else if (onlyHost == "pxl.leads.su") {
            changed = UrlFromCgi(result, "deeplink");
        } else if (onlyHost == "rdr.salesdoubler.com.ua") {
            changed = UrlFromCgi(result, "dlink");
        } else if (onlyHost == "redirect.appmetrica.yandex.com") {
            const auto pos1  = srcNoPrefix.find("back_url=http");
            if (pos1 != TStringBuf::npos) {
                result = srcNoPrefix.substr(pos1 + 9, result.size() - pos1 - 9);
                changed = true;
            }
        } else if (onlyHost == "1389598.redirect.appmetrica.yandex.com") {
            TStringBuf path;
            StringSplitter(NUrl::SplitUrlToHostAndPath(result).path).SplitBySet("?#").Take(1).CollectInto(&path);
            result = TString::Join("https://pokupki.market.yandex.ru", path);
            changed = true;
        } else if (onlyHost == "n.apclick.ru") {
            const auto pos1  = srcNoPrefix.find("url=http");
            if (pos1 != TStringBuf::npos) {
                result = CGIUnescapeRet(srcNoPrefix.substr(pos1 + 4, result.size() - pos1 - 4));
                changed = true;
            }
        } else if (onlyHost == "pricesguru.ru") {
            changed = UrlFromCgi(result, "url", true);
        } else if (onlyHost.EndsWith(".onelink.me")) {
            changed = UrlFromCgi(result, "af_web_dp");
        } else if (onlyHost.EndsWith(".adj.st")) {
            constexpr TStringBuf args[] = {
                "url",

                "adjust_fallback",
                "adjust_redirect",
                "adj_fallback",
                "adj_redirect",

                "adj_deeplink",
                "adj_deep_link",
            };

            for (const auto& arg : args) {
                if (changed) {
                    break;
                }

                changed = UrlFromCgi(result, arg);
            }
        } else {
            // To work with ssl.hurra.com/* and others.
            const auto pos1 = srcNoPrefix.find("url=[[");
            if (pos1 != TStringBuf::npos) {
                const auto pos2 = srcNoPrefix.find("]]", pos1 + 1);
                if (pos2 != TStringBuf::npos) {
                    result = srcNoPrefix.substr(pos1 + 6, pos2 - pos1 - 6);
                    changed = true;
                }
            }
            if (changed) {
                const auto pos3 = srcNoPrefix.find("||http");
                if (pos3 != TStringBuf::npos) {
                    result = srcNoPrefix.substr(pos3 + 2, result.size() - pos3 - 2);
                    changed = true;
                }
            }
        }

        if (!changed) {
            break;
        } else if (result.StartsWith("http%3A") || result.StartsWith("https%3A")) {
            result = CGIUnescapeRet(result);
        }
    }

    SubstGlobal(result, "&amp;", "&");

    // Removing UTMs.
    const size_t cgiBegin = result.find('?');
    if (cgiBegin != TString::npos) {
        const size_t cgiEnd = result.find('#', cgiBegin + 1);

        const TStringBuf cgiStr =
            (cgiEnd != TString::npos)
            ? TStringBuf(result.data() + cgiBegin + 1, result.data() + cgiEnd)
            : TStringBuf(result.data() + cgiBegin + 1, result.data() + result.size());

        TCgiParameters cgis;
        cgis.ScanAddAll(cgiStr);

        for (auto iter = cgis.begin(); iter != cgis.end();) {
            if (iter->first.StartsWith('_')
                || iter->first.StartsWith("utm-")
                || iter->first.StartsWith("utm_")
                || iter->first.StartsWith("pm_")
                || iter->first.StartsWith("ev_")
                || iter->first.StartsWith("adjust_")
                || AdsBadCgis.contains(iter->first)
                || HasBraces(iter->second))
            {
                cgis.erase(iter++);
            } else {
                ++iter;
            }
        }

        result =
            result.substr(0, cgiBegin)
            + (cgis.empty() ? "" : ("?" + cgis.Print()))
            + ((cgiEnd != TString::npos) ? result.substr(cgiEnd) : "");
    } else {
        const size_t cgiEnd = result.find('#');
        if (cgiEnd != TString::npos) {
            result = result.substr(0, cgiEnd);
        }
    }

    size_t offset = 0;
    while ((offset = result.find('{', offset)) != TString::npos) {
        const auto offset2 = result.find('}', offset);
        if (offset2 == TString::npos) {
            break;
        }
        result.replace(offset, offset2 - offset + 1, TString());
    }

    return result;
}

TString NormalizeUrlCgi(const TStringBuf url, std::function<bool(const TString&)> pred) {
    TStringBuf clearUrl, cgiParams, fragment;
    SeparateUrlFromQueryAndFragment(url, clearUrl, cgiParams, fragment);
    if (cgiParams.empty()) {
        return TString{url};
    }

    TVector<TString> normalizedParams;
    StringSplitter(cgiParams).Split('&').AddTo(&normalizedParams);
    EraseIf(normalizedParams, pred);
    TString cgis = JoinSeq("&", normalizedParams);

    const TString suffix = fragment.empty() ? "" : "#" + TString{fragment};
    if (cgis.empty()) {
        return TString::Join(TString{clearUrl}, suffix);
    }

    return TString::Join(clearUrl, "?", cgis, suffix);
}

bool IsUtmOrYrwinfo(const TString& cgiParam) {
    return cgiParam.StartsWith("utm") || cgiParam.StartsWith("yrwinfo");
}

TString RemoveCgi(const TStringBuf url) {
    TStringBuf cleanedUrl, urlCgi, urlFragment;
    SeparateUrlFromQueryAndFragment(url, cleanedUrl, urlCgi, urlFragment);
    return TString{cleanedUrl};
}

} // NUrlNorm
