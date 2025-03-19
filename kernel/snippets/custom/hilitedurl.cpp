#include "hilitedurl.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/custom/hostnames_data/hostnames.h>
#include <kernel/snippets/sent_match/extra_attrs.h>
#include <kernel/snippets/sent_match/lang_check.h>

#include <kernel/snippets/strhl/hilite_mark.h>

#include <kernel/snippets/urlcut/urlcut.h>

#include <kernel/snippets/urlmenu/common/common.h>
#include <kernel/snippets/urlmenu/cut/cut.h>
#include <kernel/snippets/urlmenu/cut/special.h>
#include <kernel/snippets/urlmenu/dump/dump.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>
#include <kernel/lemmer/core/language.h>
#include <library/cpp/stopwords/stopwords.h>

#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/generic/utility.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/string_utils/url/url.h>

namespace {
        NSnippets::W2WTrie GetDomainTrie(const NSnippets::TReplaceContext& ctx) {
            NSnippets::W2WTrie trie;
            if (NSnippets::DOMAIN_SUBSTITUTE_TRIES.contains(ctx.Cfg.GreenUrlDomainTrieKey())) {
                trie = NSnippets::DOMAIN_SUBSTITUTE_TRIES.at(ctx.Cfg.GreenUrlDomainTrieKey());
            }
            return trie;
        }
};

namespace NSnippets {
    static int CountUniqueHilitedWords(const TVector<TUtf16String>& hlWords, const TWordFilter& stopWords) {
        const TLangMask defaultLang(LANG_RUS, LANG_ENG);
        TSet<TUtf16String> uniqueWords;
        int counter = 0;
        for (const TUtf16String& hlWord : hlWords) {
            if (stopWords.IsStopWord(hlWord)) {
                continue;
            }
            TWLemmaArray lemmas;
            NLemmer::AnalyzeWord(hlWord.data(), hlWord.size(), lemmas, defaultLang);
            TSet<TUtf16String> uniqueLemmas;
            for (const TYandexLemma& lemma : lemmas) {
                uniqueLemmas.insert(TUtf16String(lemma.GetText(), lemma.GetTextLength()));
            }
            bool isNewLemma = true;
            for (const TUtf16String& uniqueLemma : uniqueLemmas) {
                if (!uniqueWords.insert(uniqueLemma).second) { // not inserted
                    isNewLemma = false;
                }
            }
            if (isNewLemma) {
                ++counter;
            }
        }
        return counter;
    }

    static bool HilitedUrlIsBetter(const NUrlCutter::THilitedString& hilitedUrl,
                                   const NUrlMenu::THilitedUrlMenu& urlMenu,
                                   const TWordFilter& stopWords) {
        if (!urlMenu.Items) {
            return true;
        }
        const int hlUrlCount = CountUniqueHilitedWords(hilitedUrl.GetHilitedWords(), stopWords);
        const int urlMenuCount = CountUniqueHilitedWords(urlMenu.GetHilitedWords(), stopWords);
        return hlUrlCount > urlMenuCount;
    }

    static TUrlMenuVector MakeUrlMenu(const TReplaceContext& ctx, const NUrlCutter::THilitedString& hilitedUrl) {
        TUrlMenuVector urlMenu;
        if (!ctx.Url) {
            return urlMenu;
        }
        auto it = ctx.DocInfos.find("urlmenu");
        if (it != ctx.DocInfos.end()) {
            NUrlMenu::Deserialize(urlMenu, it->second);
        }
        if (ctx.Cfg.GetForeignNavHackLang() == LANG_TUR) {
            for (size_t i = 1; i < urlMenu.size(); ++i) {
                const bool isTail = urlMenu[i].first.empty();
                if (!isTail && HasTooManyCyrillicWords(urlMenu[i].second, 1)) {
                    urlMenu.clear();
                    break;
                }
            }
        }
        const ELanguage lang = ctx.DocLangId != LANG_UNK ? ctx.DocLangId : LANG_RUS;
        NUrlMenu::TSpecialBuilder::Create(ctx.Url, urlMenu, lang);
        if (urlMenu) {
            NUrlMenu::THilitedUrlMenu hilitedUrlMenu(urlMenu);
            hilitedUrlMenu.SetHostTextFromUrl(ctx.Url);
            hilitedUrlMenu.HiliteAndCut(ctx.Cfg.GetMaxUrlmenuLength(), ctx.RichtreeWanderer, ctx.IH, lang, GetDomainTrie(ctx));
            hilitedUrlMenu.SetTailUrl(ctx.Url);
            if (HilitedUrlIsBetter(hilitedUrl, hilitedUrlMenu, ctx.Cfg.GetStopWordsFilter())) {
                urlMenu.clear();
            } else {
                urlMenu = hilitedUrlMenu.Merge(DEFAULT_MARKS.OpenTag, DEFAULT_MARKS.CloseTag);
            }
        }
        return urlMenu;
    }

    static NUrlCutter::THilitedString HiliteUrl(const TReplaceContext& ctx,
                                                const TString& urlToHilite,
                                                size_t maxUrlLength,
                                                size_t urlCutThreshold) {
        return NUrlCutter::HiliteAndCutUrl(
            urlToHilite,
            maxUrlLength,
            urlCutThreshold,
            ctx.RichtreeWanderer,
            ctx.DocLangId != LANG_UNK ? ctx.DocLangId : LANG_RUS,
            ctx.DocEncoding,
            true,
            GetDomainTrie(ctx));
    }

    static TString FixSearchUrl(const TString& urlSrc) {
        TString url = AddSchemePrefix(urlSrc);
        constexpr TStringBuf ESCAPED_FRAGMENT_MARKER = "_escaped_fragment_=";
        const size_t pos = url.find(ESCAPED_FRAGMENT_MARKER);
        if (pos != url.npos) {
            if (pos > 0 && (url[pos - 1] == '?' || url[pos - 1] == '&')) {
                const size_t endPos = pos + ESCAPED_FRAGMENT_MARKER.length();
                url = url.substr(0, pos - 1) + "#!" + CGIUnescapeRet(url.substr(endPos));
            }
        }
        return url;
    }

    void THilitedUrlDataReplacer::DoWork(TReplaceManager* manager) {
        const TReplaceContext& ctx = manager->GetContext();

        bool hideUrlMenu = ctx.Cfg.ExpFlagOn("hide_urlmenu");
        TString mobileUrl;
        TUtf16String hilitedMobileUrlStr;
        TUrlMenuVector urlMenu;

        if (ctx.Cfg.UseMobileUrl() && ctx.Cfg.IsTouchReport()) {
            auto it = ctx.DocInfos.find("mobile_beauty_url");
            if (it != ctx.DocInfos.end()) {
                mobileUrl = it->second;

                i32 urlReduction = (i32)ctx.Cfg.GetMobileHilitedUrlReduction();
                i32 maxUrlLength = Max((i32)ctx.Cfg.GetMaxUrlLength() - urlReduction, 0);
                i32 urlCutThreshold = Max((i32)ctx.Cfg.GetUrlCutThreshold() - urlReduction, 0);
                NUrlCutter::THilitedString hilitedMobileUrl = HiliteUrl(
                    ctx, mobileUrl, maxUrlLength, urlCutThreshold);
                hilitedMobileUrlStr = hilitedMobileUrl.Merge(DEFAULT_MARKS.OpenTag, DEFAULT_MARKS.CloseTag);

                if (!ctx.Cfg.ExpFlagOff("moburlv1")) {
                    NJson::TJsonValue features(NJson::JSON_MAP);
                    features["type"] = "mobile_beauty_url";
                    features["full_url"] = mobileUrl;
                    features["hilited_url"] = WideToUTF8(hilitedMobileUrlStr);
                    NJson::TJsonValue data(NJson::JSON_MAP);
                    data["content_plugin"] = true;
                    data["features"] = features;
                    manager->GetExtraSnipAttrs().AddClickLikeSnipJson(
                        "mobile_beauty_url", NJson::WriteJson(data));
                }

                hideUrlMenu = ctx.Cfg.HideUrlMenuForMobileUrl();
            }
        }

        NUrlCutter::THilitedString hilitedUrl = HiliteUrl(
            ctx, ctx.Url, ctx.Cfg.GetMaxUrlLength(), ctx.Cfg.GetUrlCutThreshold());
        TUtf16String hilitedUrlStr = hilitedUrl.Merge(DEFAULT_MARKS.OpenTag, DEFAULT_MARKS.CloseTag);
        manager->GetExtraSnipAttrs().SetHilitedUrl(WideToUTF8(hilitedUrlStr));
        manager->SetMarker(MRK_HILITED_URL);

        if (!hideUrlMenu) {
            urlMenu = MakeUrlMenu(ctx, hilitedUrl);
            if (urlMenu) {
                manager->GetExtraSnipAttrs().SetUrlmenu(NUrlMenu::Serialize(urlMenu));
                manager->SetMarker(MRK_URL_MENU);
            }
        }

        if (ctx.Cfg.ExpFlagOn("serpdataurl")) {
            NJson::TJsonValue urlFeatures(NJson::JSON_MAP);

            NJson::TJsonValue documentUrlFeatures(NJson::JSON_MAP);
            documentUrlFeatures["full_url"] = FixSearchUrl(ctx.Url);
            documentUrlFeatures["hilited_url"] = WideToUTF8(hilitedUrlStr);
            if (urlMenu) {
                for (auto& item : urlMenu) {
                    item.first = UTF8ToWide(FixSearchUrl(WideToUTF8(item.first)));
                }
                documentUrlFeatures["url_menu"] = NUrlMenu::SerializeToJsonValue(urlMenu);
            }
            urlFeatures["document_url"] = documentUrlFeatures;

            if (hilitedMobileUrlStr && ctx.Cfg.ExpFlagOn("moburlv2")) {
                NJson::TJsonValue mobileBeautyUrlFeatures(NJson::JSON_MAP);
                mobileBeautyUrlFeatures["full_url"] = FixSearchUrl(mobileUrl);
                mobileBeautyUrlFeatures["hilited_url"] = WideToUTF8(hilitedMobileUrlStr);
                urlFeatures["mobile_beauty_url"] = mobileBeautyUrlFeatures;
            }

            urlFeatures["type"] = "snippet_url";
            NJson::TJsonValue snippetUrlData(NJson::JSON_MAP);
            snippetUrlData["content_plugin"] = true;
            snippetUrlData["block_type"] = "construct";
            snippetUrlData["features"] = urlFeatures;
            manager->GetExtraSnipAttrs().AddClickLikeSnipJson(
                "snippet_url", NJson::WriteJson(snippetUrlData, false));
        }
    }
}
