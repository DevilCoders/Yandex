#include "productoffer.h"
#include "extended_length.h"

#include <kernel/snippets/archive/staticattr.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/custom/schemaorg_viewer/schemaorg_viewer.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/read_helper/read_helper.h>
#include <kernel/snippets/schemaorg/product_offer.h>
#include <kernel/snippets/schemaorg/schemaorg_traversal.h>
#include <kernel/snippets/sent_match/extra_attrs.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/length.h>
#include <kernel/snippets/sent_match/retained_info.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/simple_cmp/simple_cmp.h>
#include <kernel/snippets/smartcut/clearchar.h>
#include <kernel/snippets/titles/make_title/util_title.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>

#include <library/cpp/langs/langs.h>
#include <util/charset/unidata.h>
#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/mem.h>
#include <util/string/builder.h>
#include <util/string/subst.h>
#include <library/cpp/string_utils/url/url.h>

namespace NSnippets {
    static const TString HEADLINE_SRC = TString("productoffer_snip");
    static const TString HEADLINE_SRC_IMG = TString("productoffer_snip_img");

    static const TString BLACKLISTED_HOSTS[] = {
        "e96.ru", // Bad descriptions: Стиральная машина Samsung WF-8590NMW9: в наличии, гарантия 1 год
        "sidex.ru", // Bad descriptions: Купите "amazon kindle 5 :" сегодня и вы получите бонусную карту Сайдекса. Здесь вы можете прочитать отзывы о "amazon kindle 5 :", и оставить свой, будем рады любому мнению!
        "am.ru", // Bad descriptions: Am.ru - наиболее крупный автомобильный рынок в СНГ. Полная база предложений в Астрахани. Информация о моделях, отзывы и рекомендации.
    };

    static const size_t MIN_SNIP_LENGTH = 60;

    static const TUtf16String KNOWN_TAGS[] = {
        u"<br>",
        u"<br />",
        u"<strong>",
        u"</strong>",
        u"<b>",
        u"</b>",
        u"<p>",
        u"</p>",
    };

    static bool IsHTMLTrash(const TString& text) {
        // check if is "</.+>"
        size_t openTagPos = text.find("</");
        if (openTagPos != TString::npos && text.find('>', openTagPos + 3)) {
            return true;
        }

        // check if is "<.+/>
        openTagPos = text.find('<');
        if (openTagPos != TString::npos && text.find("/>", openTagPos + 2)) {
            return true;
        }

        // check if hash substring of "&[#[::alnum::]]+;" format
        int sequenceBegin = -1;
        for (int i = 0; i < int(text.length()); ++i) {
            if (text[i] == '&') {
                sequenceBegin = i;
            } else if (text[i] == ';' && sequenceBegin >= 0 && i > sequenceBegin + 1) {
                return true;
            } else if (!isalnum(text[i]) && text[i] != '#') {
                sequenceBegin = -1;
            }
        }
        return false;
    }

    static void ClearTags(TUtf16String& str) {
        // clear known html code
        for (const TUtf16String& tag : KNOWN_TAGS) {
            SubstGlobal(str, tag, TUtf16String());
        }
        SubstGlobal(str, u"&quot;", u"\"");
        // ban the rest html code
        if (IsHTMLTrash(WideToUTF8(str))) {
            str.clear();
        }
    }

    static bool IsFromHost(TStringBuf urlHost, TStringBuf host) {
        if (!urlHost.EndsWith(host)) {
            return false;
        }
        return urlHost == host || urlHost[urlHost.size() - host.size() - 1] == '.';
    }

    static bool IsBlacklisted(TStringBuf host) {
        for (const TString& s : BLACKLISTED_HOSTS) {
            if (IsFromHost(host, s)) {
                return true;
            }
        }
        return false;
    }

    static TUtf16String FormatOfferTitle(const NSchemaOrg::TOffer& offer) {
        TUtf16String title = offer.ProductName;
        if (title && title.back() == '.') {
            title.remove(title.size() - 1);
        }
        if (title) {
            ClearTags(title);
        }
        return title;
    }

    static TUtf16String FormatOfferDesc(const NSchemaOrg::TOffer& offer) {
        TUtf16String desc = JoinStrings(offer.ProductDesc.begin(), offer.ProductDesc.end(), u" ");
        if (!desc) {
            desc = JoinStrings(offer.OfferDesc.begin(), offer.OfferDesc.end(), u" ");
        }
        if (desc) {
            ClearTags(desc);
            ClearChars(desc, true);
        }
        return desc;
    }

    static NSchemaOrg::TPriceParsingResult GetPrice(const NSchemaOrg::TOffer& offer, TStringBuf host, const TDocInfos& docInfos) {
        NSchemaOrg::TPriceParsingResult price;
        if (!offer.OfferIsNotAvailable() && !offer.PriceValidUntil && !offer.ValidThrough) {
            TStaticData marketData(docInfos, "market");
            bool hasMarketPrice = marketData.Attrs.contains("price");
            if (!hasMarketPrice) {
                return offer.ParsePrice(host);
            }
        }
        return price;
    }

    static TUtf16String FormatOfferDescPrefix(const NSchemaOrg::TOffer& offer, TStringBuf host, ELanguage langId, const TDocInfos& docInfos, bool separatePrice) {
        TUtf16String prefix;
        bool isAvito = (IsFromHost(host, "avito.ru") || IsFromHost(host, "torg.ua")) && langId == LANG_RUS;
        if (isAvito && !offer.AvailableAtOrFrom.empty()) {
            prefix += u"Город: " + offer.AvailableAtOrFrom.back() + u" ";
        }
        if (!separatePrice) {
            NSchemaOrg::TPriceParsingResult price = GetPrice(offer, host, docInfos);
            if (price.IsValid) {
                TUtf16String formattedPrice = price.FormatPrice(langId);
                if (formattedPrice && formattedPrice.back() != '.') {
                    formattedPrice += '.';
                }
                prefix += formattedPrice;
                prefix += ' ';
            }
        }
        return prefix;
    }

    static bool IsTrash(const TReplaceContext& ctx, const TUtf16String& text, bool isAlgoSnip) {
        TReadabilityChecker checker(ctx.Cfg, ctx.QueryCtx, ctx.DocLangId);
        checker.CheckShortSentences = true;
        checker.CheckBadCharacters = true;
        checker.CheckLanguageMatch = true;
        checker.CheckCapitalization = true;
        return !checker.IsReadable(text, isAlgoSnip);
    }

    float CalcTextLength(const TUtf16String& text, const TReplaceContext& ctx) {
        if (!text) {
            return 0.0f;
        }
        TRetainedSentsMatchInfo customSents;
        customSents.SetView(text, TRetainedSentsMatchInfo::TParams(ctx.Cfg, ctx.QueryCtx));
        int wc = customSents.GetSentsMatchInfo()->WordsCount();
        if (wc == 0) {
            return 0.0f;
        }
        return ctx.SnipWordSpanLen.CalcLength(TSingleSnip(0, wc - 1, *customSents.GetSentsMatchInfo()));
    }

    TProductOfferReplacer::TProductOfferReplacer(const TSchemaOrgArchiveViewer& arcViewer)
        : IReplacer("product_offer")
        , ArcViewer(arcViewer)
    {
    }

    void TProductOfferReplacer::DoWork(TReplaceManager* manager) {
        const TReplaceContext& ctx = manager->GetContext();

        const NSchemaOrg::TOffer* offer = ArcViewer.GetOffer();
        if (!offer) {
            return;
        }
        if (offer->ErrorMessage) {
            manager->ReplacerDebug(offer->ErrorMessage);
            return;
        }

        if (ctx.Cfg.IsMainPage()) {
            manager->ReplacerDebug("the main page is not processed");
            return;
        }

        TStringBuf host = GetOnlyHost(ctx.Url);
        if (IsBlacklisted(host)) {
            manager->ReplacerDebug("the host is blacklisted");
            return;
        }

        TUtf16String desc = FormatOfferDesc(*offer);
        if (!desc) {
            manager->ReplacerDebug("Product/description and Offer/description fields are missing");
            return;
        }

        bool separatePrice = ctx.Cfg.ExpFlagOn("separate_price");
        TUtf16String descPrefix = FormatOfferDescPrefix(*offer, host, ctx.DocLangId, ctx.DocInfos, separatePrice);

        TUtf16String title = FormatOfferTitle(*offer);
        TSnipTitle newTitle = MakeSpecialTitle(title, ctx.Cfg, ctx.QueryCtx);
        if (ctx.NaturalTitle.GetTitleSnip().HasMatches() &&
            !newTitle.GetTitleSnip().HasMatches())
        {
            manager->ReplacerDebug("Product/name field is missing or has no matches. "
                "The markup is considered to be irrelevant",
                TReplaceResult().UseText(TUtf16String(), HEADLINE_SRC).UseTitle(newTitle));
            return;
        }
        double oldTitleWeight = TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true, true).Add(ctx.NaturalTitle).GetWeight();
        double newTitleWeight = TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true, true).Add(newTitle).GetWeight();
        bool replaceTitle = oldTitleWeight < newTitleWeight;

        TUtf16String newText;
        float prefixLength = CalcTextLength(descPrefix, ctx);
        float maxDescLength = ctx.LenCfg.GetMaxTextSpecSnipLen() - prefixLength;
        bool isAlgoSnip = true;
        TUtf16String cutDesc = CutSnip(desc, ctx.Cfg, ctx.QueryCtx, maxDescLength, maxDescLength, nullptr, ctx.SnipWordSpanLen);
        TSmartCutOptions options(ctx.Cfg);
        if (cutDesc) {
            newText = descPrefix + cutDesc;
            options.MaximizeLen = true;
        } else {
            isAlgoSnip = false;
            newText = descPrefix + desc;
        }

        float maxLen = ctx.LenCfg.GetMaxTextSpecSnipLen();
        float maxExtLen = GetExtSnipLen(ctx.Cfg, ctx.LenCfg);
        const auto& extSnip = SmartCutWithExt(newText, ctx.IH, maxLen, maxExtLen, options);
        newText = extSnip.Short;

        TReplaceResult res;
        res.UseText(extSnip, HEADLINE_SRC);

        TUtf16String oldText = ctx.Snip.GetRawTextWithEllipsis();

        if (isAlgoSnip && newText.size() < MIN_SNIP_LENGTH) {
            manager->ReplacerDebug((TStringBuilder() << "the text is shorter than " << MIN_SNIP_LENGTH << " characters"), res);
            return;
        }
        if (!isAlgoSnip && newText.size() * 3 <= oldText.size() * 2) {
            manager->ReplacerDebug("the text is shorter than 2/3 of the original snippet length", res);
            return;
        }
        if (IsTrash(ctx, newText, isAlgoSnip)) {
            manager->ReplacerDebug("the text is recognized as unreadable", res);
            return;
        }

        double oldSnipWeight = TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true).Add(ctx.SuperNaturalTitle).Add(ctx.Snip).GetWeight();
        double newSnipWeight = TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true).Add(replaceTitle ? newTitle : ctx.NaturalTitle).Add(newText).GetWeight();
        if (newSnipWeight < oldSnipWeight && replaceTitle) {
            replaceTitle = false;
            newSnipWeight = TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true).Add(ctx.NaturalTitle).Add(newText).GetWeight();
        }
        if (replaceTitle) {
            res.UseTitle(newTitle);
        }
        if (newSnipWeight < oldSnipWeight) {
            manager->ReplacerDebug("contains less non stop query words than original one", res);
            return;
        }

        auto attr = ctx.DocInfos.find("foto_product");
        if (attr != ctx.DocInfos.end() && ctx.Cfg.UseProductOfferImage()) {
            TString thumbUrl;
            TMemoryInput buf(TStringBuf(attr->second));
            NJson::TJsonValue attrVal;
            if (NJson::ReadJsonTree(&buf, &attrVal)) {
                const NJson::TJsonValue* photoVal = nullptr;
                if (attrVal.GetValuePointer("photo", &photoVal)) {
                    const NJson::TJsonValue* itemVal = nullptr;
                    if (photoVal->GetValuePointer(0, &itemVal)) {
                        thumbUrl = itemVal->GetString();
                    }
                }
            }
            if (thumbUrl) {
                NJson::TJsonValue features(NJson::JSON_MAP);
                features["thumb"] = thumbUrl;
                features["type"] = "foto_product";
                NJson::TJsonValue data(NJson::JSON_MAP);
                data["content_plugin"] = true;
                data["block_type"] = "construct";
                data["features"] = features;
                manager->GetExtraSnipAttrs().AddClickLikeSnipJson("foto_product", NJson::WriteJson(data));
                res.SetTextSrc(HEADLINE_SRC_IMG);
            }
        }

        if (separatePrice) {
            NSchemaOrg::TPriceParsingResult price = GetPrice(*offer, host, ctx.DocInfos);
            if (price.IsValid) {
                NJson::TJsonValue features(NJson::JSON_MAP);
                features["type"] = "snip_price";
                features["price"] = price.ParsedPriceMul100 / 100.;
                features["currency"] = price.GetCurrencyCodeWithRur();
                features["is_low_price"] = price.IsLowPrice;

                NJson::TJsonValue data(NJson::JSON_MAP);
                data["content_plugin"] = true;
                data["type"] = "snip_price";
                data["block_type"] = "construct";
                data["features"] = features;
                manager->GetExtraSnipAttrs().AddClickLikeSnipJson("snip_price", NJson::WriteJson(data));
            }
        }

        manager->Commit(res, MRK_PRODUCT_OFFER);
    }
}
