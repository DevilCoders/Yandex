#include "special.h"

#include <kernel/snippets/urlcut/decode_url.h>
#include <kernel/snippets/urlcut/consts.h>

#include <util/string/subst.h>
#include <library/cpp/string_utils/url/url.h>

namespace NUrlMenu {
    static constexpr TStringBuf TWITTER_HOST = "twitter.com";
    static constexpr TStringBuf WIKI_HOST = ".wikipedia.org";

    static bool IncrementSafe(size_t& position, size_t length) {
        if (position != TString::npos && position + 1 < length) {
            ++position;
            return true;
        }
        return false;
    }

    static bool HasSubstr(const TString& str, const TString& substr, size_t from, size_t posLessThen) {
        return str.find(substr, from) < posLessThen;
    }

    static TUrlMenuVector CreateForTwitter(const TString& url, ELanguage lang) {
        TUrlMenuVector urlMenu;

        size_t tailStart = url.rfind(NUrlCutter::PATH_SEP_CHAR);
        size_t hostnameEnd = url.find(NUrlCutter::PATH_SEP_CHAR, GetHttpPrefixSize(url));
        Y_ASSERT(hostnameEnd <= url.length());

        if (tailStart < hostnameEnd || tailStart == TString::npos) {
            return TUrlMenuVector();
        }

        size_t tailNestingLevel =
            std::count(
                url.begin() + hostnameEnd,
                url.begin() + tailStart,
                NUrlCutter::PATH_SEP_CHAR);

        if (hostnameEnd + 1 < url.length()) {
            ++hostnameEnd; // add "/" to host address;
        }

        if (tailNestingLevel == 1 &&
            (HasSubstr(url, NUrlCutter::UGLY_AJAX_CGI_SHARP_EXCLAMATION, hostnameEnd, tailStart) ||
             HasSubstr(url, NUrlCutter::UGLY_AJAX_AMP_SHARP_EXCLAMATION, hostnameEnd, tailStart) ||
             HasSubstr(url, NUrlCutter::SHARP_EXCLAMATION, hostnameEnd, tailStart))) {
            typedef std::pair<TUtf16String, TUtf16String> TWidePair;
            urlMenu.push_back(
                TWidePair(ASCIIToWide(url.substr(0, hostnameEnd)), ASCIIToWide(GetOnlyHost(url))));

            if (!IncrementSafe(tailStart, url.length())) {
                return TUrlMenuVector();
            }
            TUtf16String tailName = NUrlCutter::DecodeUrlPath(url.substr(tailStart), lang);
            urlMenu.push_back(TWidePair(ASCIIToWide(url), tailName));
        }

        return urlMenu;
    }

    static void CreateForWiki(const TString& url, ELanguage lang, TUrlMenuVector& res) {
        size_t tailStart = url.rfind(NUrlCutter::PATH_SEP_CHAR);
        size_t hostnameEnd = url.find(NUrlCutter::PATH_SEP_CHAR, GetHttpPrefixSize(url));
        if (!IncrementSafe(tailStart, url.length())) {
            return;
        }
        if (tailStart < hostnameEnd || tailStart == TString::npos) {
            return;
        }
        if (hostnameEnd + 1 < url.length()) {
            ++hostnameEnd; // add "/" to host address;
        }
        size_t tailEnd = url.find(NUrlCutter::CGI_SEP_CHAR, tailStart);
        if (tailEnd == TString::npos) {
            tailEnd = url.size();
        }
        TUtf16String tailName = NUrlCutter::DecodeUrlPath(url.substr(tailStart, tailEnd - tailStart), lang);

        SubstGlobal(tailName, wchar16('_'), wchar16(' '));

        if (tailName.size()) {
            res.clear();
            res.push_back(std::make_pair(ASCIIToWide(url.substr(0, hostnameEnd)), ASCIIToWide(GetOnlyHost(url))));
            res.push_back(std::make_pair(ASCIIToWide(url), tailName));
        }
    }

    void TSpecialBuilder::Create(const TString& url, TUrlMenuVector& res, ELanguage lang) {
        TStringBuf host = GetOnlyHost(url);
        if (res.empty() && host == TWITTER_HOST) {
            CreateForTwitter(url, lang).swap(res);
        } else if (host.EndsWith(WIKI_HOST)) {
            CreateForWiki(url, lang, res);
        }
    }
}
