#include "preview.h"
#include "mediawiki.h"

#include <kernel/snippets/archive/staticattr.h>
#include <kernel/snippets/archive/view/view.h>

#include <kernel/snippets/config/config.h>

#include <kernel/snippets/custom/forums_handler/forums_handler.h>
#include <kernel/snippets/custom/opengraph/og.h>
#include <kernel/snippets/custom/preview_viewer/preview_viewer.h>
#include <kernel/snippets/custom/sea_json/raw_preview_fillers.h>

#include <kernel/snippets/cut/cut.h>

#include <kernel/snippets/qtree/query.h>

#include <kernel/snippets/schemaorg/proto/schemaorg.pb.h>
#include <kernel/snippets/schemaorg/schemaorg_serializer.h>
#include <kernel/snippets/schemaorg/product_offer.h>
#include <kernel/snippets/schemaorg/schemaorg_traversal.h>

#include <kernel/snippets/sent_match/extra_attrs.h>
#include <kernel/snippets/sent_match/glue.h>
#include <kernel/snippets/sent_match/lang_check.h>
#include <kernel/snippets/sent_match/length.h>
#include <kernel/snippets/sent_match/retained_info.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/similarity.h>
#include <kernel/snippets/sent_match/single_snip.h>
#include <kernel/snippets/sent_match/tsnip.h>

#include <kernel/snippets/sent_info/beautify.h>
#include <kernel/snippets/sent_info/sent_info.h>

#include <kernel/snippets/smartcut/consts.h>
#include <kernel/snippets/smartcut/cutparam.h>

#include <kernel/snippets/strhl/zonedstring.h>
#include <kernel/snippets/strhl/goodwrds.h>

#include <kernel/snippets/titles/make_title/util_title.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/json/json_reader.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/charset/ci_string.h>
#include <util/datetime/base.h>
#include <util/generic/string.h>
#include <util/generic/list.h>
#include <util/string/cast.h>
#include <util/string/printf.h>
#include <util/string/subst.h>
#include <util/string/type.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/draft/date.h>

namespace NSnippets {
namespace {

    const TStringBuf BLACK_LIST[] = {
        "vibirai.ru",
    };
    const TStringBuf WHITE_LIST[] = {
        "wikimart.ru",
        "fishretail.ru",
        "sotmarket.ru",
        "grainboard.ru",
        "labirint.ru",
        "tovar-x.ru",
        "milknet.ru",
        "krasota-x.ru",
        "sweetinfo.ru",
        "drinkinfo.ru",
        "ozon.ru",
        "auto.yandex.ru",
    };

    struct TSchemaObj {
        struct TPrice {
            TString PriceMin;
            TString PriceMinCent;
            TString PriceMax;
            TString PriceMaxCent;
            TString Currency;
        };
        struct TRating {
            TString RatingValue;
            TString RatingBest;
            TString RatingCount;
        };
        struct TReview {
            TString DatePublished;
            TString ReviewAuthor;
            TString ReviewBody;
        };

        TString Type;
        TString Name;
        TString Description;
        TString Url;
        TVector<TString> Images;
        TVector<std::pair<TString, TString> > Props;
        TVector<std::pair<TString, TString> > MainProps;
        TPrice Price;
        TRating Rating;
        TVector<TReview> Reviews;
        TString ReviewCount;
        bool BadContent;
        bool VeryBadContent;
        bool EdgeContent;

        TString GetId() const {
            TString res;
            res += Type + '\0';
            res += Name + '\0';
            res += Description + '\0';
            res += Url + '\0';
            res += ToString(Images.size()) + '\0';
            for (const auto& image : Images) {
                res += image + '\0';
            }
            res += ToString(Props.size()) + '\0';
            for (const auto& prop : Props) {
                res += prop.first + '\0';
                res += prop.second + '\0';
            }
            res += ToString(MainProps.size()) + '\0';
            for (const auto& prop : MainProps) {
                res += prop.first + '\0';
                res += prop.second + '\0';
            }
            res += Price.PriceMin + '\0';
            res += Price.PriceMinCent + '\0';
            res += Price.PriceMax + '\0';
            res += Price.PriceMaxCent + '\0';
            res += Price.Currency + '\0';
            res += Rating.RatingValue + '\0';
            res += Rating.RatingBest + '\0';
            res += Rating.RatingCount + '\0';
            res += ToString(Reviews.size()) + '\0';
            for (const auto& review : Reviews) {
                res += review.DatePublished + '\0';
                res += review.ReviewAuthor + '\0';
                res += review.ReviewBody + '\0';
            }
            res += ReviewCount + '\0';
            res += ToString(BadContent) + '\0';
            res += ToString(VeryBadContent) + '\0';
            res += ToString(EdgeContent) + '\0';
            return res;
        }
    };

    TDate GetDateFromString(const TUtf16String& s) { // format DD.MM.YYYY
        TDate res((TDate()));
        if (s.size() != 10)
            return res;
        for (size_t i = 0; i < s.size(); ++i) {
            if (i == 2 || i == 5) {
                if (s[i] != '.') {
                    return res;
                }
            } else if (!IsCommonDigit(s[i])) {
                return res;
            }
        }
        unsigned int day = FromString(TUtf16String(s.data(), 2));
        unsigned int month = FromString(TUtf16String(s.data() + 3, 2));
        unsigned int year = FromString(TUtf16String(s.data() + 6, 4));
        res = TDate(year, month, day);
        return res;
    }

    struct TForumMessageHelper {
        const TForumMessageZone* Zone;
        TDate Date;

        bool operator<(const TForumMessageHelper& mess) const {
            if (Date == mess.Date) {
                return Zone->Span.SentBeg < mess.Zone->Span.SentBeg;
            } else {
                return Date < mess.Date;
            }
        }
    };

    struct TForumMessageText: public TRetainedSentsMatchInfo
    {
        const TForumMessageZone* Zone;
        TDate Date;
        TUtf16String FinalText;

        TForumMessageText()
            : Zone(nullptr)
            , Date()
            , FinalText()
        {
        }
    };

    bool HasBadRange(const NSchemaOrg::TTreeNode& treeNode, const THashSet<int>& mainContent) {
        if (treeNode.HasSentBegin() && treeNode.HasSentCount()) {
            for (size_t i = 0; i < treeNode.GetSentCount(); ++i) {
                if (mainContent.find(treeNode.GetSentBegin() + i) == mainContent.end()) {
                    return true;
                }
            }
        }
        return false;
    }

    bool IsEdgeContent(const NSchemaOrg::TTreeNode& n, int guessedSentCount) {
        if (guessedSentCount && n.HasSentBegin() && n.HasSentCount() && n.GetSentCount()) {
            if ((int)n.GetSentBegin() * 10 > guessedSentCount * 8) {
                return true;
            }
            if ((int)(n.GetSentBegin() + n.GetSentCount() - 1) * 10 < guessedSentCount * 2) {
                return true;
            }
        }
        return false;
    }

    bool FineSchemaOrgCover(const TVector<const NSchemaOrg::TTreeNode*>& v, int guessedSentCount) {
        if (!guessedSentCount) {
            return true;
        }
        TVector< std::pair<int, int> > spans;
        for (size_t i = 0; i < v.size(); ++i) {
            if (v[i]->HasSentBegin() && v[i]->HasSentCount() && v[i]->GetSentCount()) {
                spans.push_back(std::make_pair(v[i]->GetSentBegin(), v[i]->GetSentCount()));
            }
        }
        spans.push_back(std::make_pair(guessedSentCount, 1));
        Sort(spans.begin(), spans.end());
        int open = 0;
        int total = 0;
        for (size_t i = 0; i < spans.size(); ++i) {
            if (spans[i].first > open && spans[i].first <= guessedSentCount) {
                total += spans[i].first - open + 1;
            }
            if (spans[i].first + spans[i].second > open) {
                open = spans[i].first + spans[i].second;
            }
        }
        return total * 10 >= guessedSentCount * 2;
    }

    bool IsBadPunct(wchar16 c) {
        return c == '.' || c == ',' || c == '!' || c == '?' || c == ':' || c == ';';
    }

    TString FixPropKey(TString s) {
        TUtf16String w = UTF8ToWide(s);
        size_t i = w.size();
        while (i > 0 && (IsSpace(w.data() + (i - 1), 1) || IsBadPunct(w[i - 1]))) {
            --i;
        }
        if (i == w.size()) {
            return s;
        }
        return WideToUTF8(w.substr(0, i));
    }

    bool LooksLikeHtml(const TString& s) {
        size_t n = 0;
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '<' || s[i] == '>' || s[i] == '&' || s[i] == '=' || s[i] == '/') {
                ++n;
            }
        }
        return n >= 10 && n * 10 >= s.size();
    }

    bool SameNameDescr(const TString& name, const TString& descr) {
        size_t p = 0;
        while (p < name.size() && p < descr.size() && name[p] == descr[p]) {
            ++p;
        }
        size_t m = name.size() < descr.size() ? name.size() : descr.size();
        return p >= m - 10 || p * 10 > m * 9 || p >= 100;
    }

    TString MakeDoubleBR(TStringBuf s) {
        TString res;
        bool first = true;
        while (s.size()) {
            TStringBuf p = s.NextTok('\n');
            if (!first) {
                res += '\n';
                res += '\n';
            }
            res += p;
            first = false;
        }
        return res;
    }

    void ParseAndLocalizeAvailability(const TString& availability,
            ELanguage docLang, TSchemaObj& o) {
        NSchemaOrg::EItemAvailability itemAvailability =
            NSchemaOrg::ParseAvailability(UTF8ToWide(availability));
        if (docLang == LANG_RUS) {
            TString value;
            if (itemAvailability == NSchemaOrg::AVAIL_IN_STOCK) {
                value = "Есть в наличии";
            } else if (itemAvailability == NSchemaOrg::AVAIL_OUT_OF_STOCK) {
                value = "Нет в наличии";
            } else if (itemAvailability == NSchemaOrg::AVAIL_PRE_ORDER) {
                value = "Под заказ";
            }
            if (value) {
                o.Props.push_back(std::make_pair(TString("Наличие"), value));
            }
        }
    }

    void ParseAndLocalizeOfferCount(const TString& offerCount,
            ELanguage docLang, TSchemaObj& o) {
        int count;
        if (TryFromString(offerCount, count) && count > 0) {
            if (docLang == LANG_RUS) {
                o.Props.push_back(std::make_pair(TString("Количество предложений"), ToString(count)));
            }
        }
    }

    void ParseAndLocalizeAvailableAtOrFrom(const NSchemaOrg::TTreeNode* offerNode,
            TStringBuf host, ELanguage docLang, TSchemaObj& o) {
        bool isAvito = host.EndsWith("avito.ru") || host.EndsWith("torg.ua");
        if (isAvito && docLang == LANG_RUS) {
            TString place;
            const NSchemaOrg::TTreeNode* placeNode =
                NSchemaOrg::FindSingleItemprop(offerNode, "availableatorfrom");
            for (size_t i = 0; placeNode && i < placeNode->NodeSize(); ++i) {
                const NSchemaOrg::TTreeNode* n = &placeNode->GetNode(i);
                if (IsPropertyName(*n, "name") && n->HasText()) {
                    if (place) {
                        place += ", ";
                    }
                    place += n->GetText();
                }
            }
            if (place) {
                o.Props.push_back(std::make_pair(TString("Город"), place));
            }
        }
    }

    void ParsePropertiesList(const NSchemaOrg::TTreeNode* listNode, TSchemaObj& o) {
        for (size_t k = 0; k < listNode->NodeSize(); ++k) {
            const NSchemaOrg::TTreeNode* elemNode = &listNode->GetNode(k);
            if (IsPropertyName(*elemNode, "itemlistelement")) {
                TString key;
                TString val;
                bool isMain = false;
                for (size_t l = 0; l < elemNode->NodeSize(); ++l) {
                    const NSchemaOrg::TTreeNode* n = &elemNode->GetNode(l);
                    if (!n->ItemtypesSize() && n->ItempropsSize()) {
                        if (n->HasText() && IsPropertyName(*n, "name")) {
                            key = n->GetText();
                        } else if (n->HasText() && IsPropertyName(*n, "value")) {
                            val = n->GetText();
                        } else if (IsPropertyName(*n, "main") && n->GetText() == TCiString("true")) {
                            isMain = true;
                        }
                    }
                }
                key = FixPropKey(key);
                if (key.size() && val.size()) {
                    if (isMain) {
                        o.MainProps.push_back(std::make_pair(key, val));
                    } else {
                        o.Props.push_back(std::make_pair(key, val));
                    }
                }
            }
        }
    }

    TString FindSingleItempropText(const NSchemaOrg::TTreeNode* node, const TString& name) {
        const NSchemaOrg::TTreeNode* res = NSchemaOrg::FindSingleItemprop(node, name);
        if (res && res->HasText()) {
            return res->GetText();
        }
        return TString();
    }

    TString FindFirstItempropText(const NSchemaOrg::TTreeNode* node, const TString& name) {
        const NSchemaOrg::TTreeNode* res = NSchemaOrg::FindFirstItemprop(node, name);
        if (res && res->HasText()) {
            return res->GetText();
        }
        return TString();
    }

    TString FindFirstItempropDate(const NSchemaOrg::TTreeNode* node, const TString& name) {
        const NSchemaOrg::TTreeNode* res = NSchemaOrg::FindFirstItemprop(node, name);
        if (res && res->HasDatetime()) {
            return res->GetDatetime();
        }
        if (res && res->HasText()) {
            return res->GetText();
        }
        return TString();
    }

    TString ParseRatingValue(const TString& ratingStr) {
        TString str = ratingStr;
        SubstGlobal(str, ',', '.');
        double ratingValue = 0.0;
        if (TryFromString(str, ratingValue) && ratingValue > 0.0) {
            ui32 val = static_cast<ui32>(ratingValue * 10.0 + 0.500001);
            if (val % 10 == 0) {
                return ToString(val / 10);
            } else {
                return ToString(val / 10) + "." + ToString(val % 10);
            }
        }
        return TString();
    }

    void ParseRating(const NSchemaOrg::TTreeNode* productNode,
            const NSchemaOrg::TTreeNode* offerNode, TSchemaObj& o) {
        const NSchemaOrg::TTreeNode* ratingNode =
            NSchemaOrg::FindFirstItemprop(productNode, "aggregaterating");
        if (!ratingNode) {
            ratingNode = NSchemaOrg::FindFirstItemprop(offerNode, "aggregaterating");
        }
        if (!ratingNode) {
            return;
        }
        TString ratingCountStr =
            FindSingleItempropText(ratingNode, "ratingcount");
        if (!ratingCountStr) {
            ratingCountStr = FindSingleItempropText(ratingNode, "reviewcount");
        }
        ui32 ratingCountInt = 0;
        TString ratingCount;
        if (TryFromString(ratingCountStr, ratingCountInt)) {
            if (ratingCountInt == 0) {
                return;
            }
            ratingCount = ToString(ratingCountInt);
        }
        TString ratingValue = ParseRatingValue(
            FindSingleItempropText(ratingNode, "ratingvalue"));
        TString ratingBest = ParseRatingValue(
            FindSingleItempropText(ratingNode, "bestrating"));
        if (ratingValue && ratingBest) {
            o.Rating.RatingValue = ratingValue;
            o.Rating.RatingBest = ratingBest;
            o.Rating.RatingCount = ratingCount;
        }
    }

    TString ParseDate(const TString& iso8601Date) {
        time_t utcTime;
        if (!ParseISO8601DateTimeDeprecated(iso8601Date.data(), utcTime)) {
            return TString();
        }
        struct tm t;
        GmTimeR(&utcTime, &t);
        return Sprintf("%d.%02d.%d", t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
    }

    void ParseReviews(const NSchemaOrg::TTreeNode* productNode, TSchemaObj& o) {
        const size_t MAX_REVIEWS = 3;
        TList<const NSchemaOrg::TTreeNode*> reviewNodes =
            NSchemaOrg::FindAllItemprops(productNode, "review", true);
        if (reviewNodes.empty()) {
            return;
        }
        size_t reviewCount = 0;
        for (const NSchemaOrg::TTreeNode* n : reviewNodes) {
            TString reviewBody = FindFirstItempropText(n, "reviewbody");
            if (!reviewBody) {
                reviewBody = FindFirstItempropText(n, "description");
            }
            if (reviewBody) {
                ++reviewCount;
                if (o.Reviews.size() < MAX_REVIEWS) {
                    o.Reviews.resize(o.Reviews.size() + 1);
                    o.Reviews.back().ReviewBody = reviewBody;
                    TString author = FindFirstItempropText(n, "author");
                    if (author) {
                        o.Reviews.back().ReviewAuthor = author;
                    }
                    TString datePublished = ParseDate(FindFirstItempropDate(n, "datepublished"));
                    if (datePublished) {
                        o.Reviews.back().DatePublished = datePublished;
                    }
                }
            }
        }
        if (reviewCount > 0) {
            o.ReviewCount = ToString(reviewCount);
        }
    }

    const NSchemaOrg::TTreeNode* FindOfferNode(const NSchemaOrg::TTreeNode* productNode) {
        const NSchemaOrg::TTreeNode* aggregateOfferNode = nullptr;
        const NSchemaOrg::TTreeNode* offerNode = nullptr;
        size_t aggregateOfferCount = 0;
        size_t offerCount = 0;
        for (size_t i = 0; i < productNode->NodeSize(); ++i) {
            const NSchemaOrg::TTreeNode* n = &productNode->GetNode(i);
            if (IsPropertyName(*n, "offers")) {
                if (IsItemOfType(*n, "aggregateoffer")) {
                    aggregateOfferNode = n;
                    ++aggregateOfferCount;
                } else if (IsItemOfType(*n, "offer")) {
                    offerNode = n;
                    ++offerCount;
                }
            }
        }
        if (aggregateOfferCount == 1) {
            return aggregateOfferNode;
        }
        if (offerCount == 1) {
            return offerNode;
        }
        return nullptr;
    }

    void ParseProduct(const NSchemaOrg::TTreeNode* productNode, TStringBuf host,
            ELanguage docLang, TSchemaObj& o) {
        const NSchemaOrg::TTreeNode* offerNode = FindOfferNode(productNode);
        if (!offerNode) {
            return;
        }
        if (!o.Description) {
            TString offerDesc = FindFirstItempropText(offerNode, "description");
            if (offerDesc && !LooksLikeHtml(offerDesc)) {
                o.Description = offerDesc;
            }
        }
        if (IsItemOfType(*offerNode, "aggregateoffer")) {
            TString lowPrice = FindSingleItempropText(offerNode, "lowprice");
            if (lowPrice) {
                TString priceCurrency = FindSingleItempropText(offerNode, "pricecurrency");
                NSchemaOrg::TPriceParsingResult low =
                    NSchemaOrg::ParsePrice(UTF8ToWide(lowPrice), UTF8ToWide(priceCurrency), host);
                if (low.IsValid) {
                    o.Price.Currency = low.GetCurrencyCodeWithRur();
                    o.Price.PriceMin = low.GetPriceIntegerPart();
                    o.Price.PriceMinCent = low.GetPriceFractionalPart();
                    TString highPrice = FindSingleItempropText(offerNode, "highprice");
                    if (highPrice) {
                        NSchemaOrg::TPriceParsingResult high =
                            NSchemaOrg::ParsePrice(UTF8ToWide(highPrice), UTF8ToWide(priceCurrency), host);
                        if (high.IsValid && high.CurrencyCode == low.CurrencyCode) {
                            o.Price.PriceMax = high.GetPriceIntegerPart();
                            o.Price.PriceMaxCent = high.GetPriceFractionalPart();
                        }
                    }
                }
            }
            ParseAndLocalizeOfferCount(
                FindSingleItempropText(offerNode, "offercount"), docLang, o);
        } else {
            TString price = FindSingleItempropText(offerNode, "price");
            if (price) {
                if (!NSchemaOrg::FindFirstItemprop(offerNode, "validthrough") &&
                    !NSchemaOrg::FindFirstItemprop(offerNode, "pricevaliduntil"))
                {
                    TString priceCurrency = FindSingleItempropText(offerNode, "pricecurrency");
                    NSchemaOrg::TPriceParsingResult res =
                        NSchemaOrg::ParsePrice(UTF8ToWide(price), UTF8ToWide(priceCurrency), host);
                    if (res.IsValid) {
                        o.Price.Currency = res.GetCurrencyCodeWithRur();
                        if (res.IsLowPrice) {
                            o.Price.PriceMin = res.GetPriceIntegerPart();
                            o.Price.PriceMinCent = res.GetPriceFractionalPart();
                        } else {
                            o.Price.PriceMax = res.GetPriceIntegerPart();
                            o.Price.PriceMaxCent = res.GetPriceFractionalPart();
                        }
                    }
                }
            }
            ParseAndLocalizeAvailability(
                FindFirstItempropText(offerNode, "availability"), docLang, o);
            ParseAndLocalizeAvailableAtOrFrom(offerNode, host, docLang, o);
            ParseRating(productNode, offerNode, o);
            ParseReviews(productNode, o);
        }
    }

    void ParseObjs(TVector<TSchemaObj>& res, const NSchemaOrg::TTreeNode* root,
            const THashSet<int>& mainContent, const THashSet<int>& goodContent,
            int guessedSentCount, bool& fineCover, TStringBuf host,
            ELanguage docLang) {
        TVector<const NSchemaOrg::TTreeNode*> v(1, root);
        for (size_t i = 0; i < v.size(); ++i) {
            TSchemaObj o;
            o.BadContent = HasBadRange(*v[i], mainContent);
            o.VeryBadContent = HasBadRange(*v[i], goodContent);

            for (const auto& child : v[i]->GetNode()) {
                const NSchemaOrg::TTreeNode* n = &child;
                v.push_back(n);
                if (HasBadRange(*n, mainContent)) {
                    o.BadContent = true;
                }
                if (HasBadRange(*n, goodContent)) {
                    o.VeryBadContent = true;
                }
                o.EdgeContent = IsEdgeContent(*n, guessedSentCount);
                if (!n->ItemtypesSize() && n->ItempropsSize()) {
                    if (IsPropertyName(*n, "name")) {
                        if (n->HasText() && n->GetText().find('<') == TString::npos &&
                                n->GetText().find('>') == TString::npos) {
                            o.Name = n->GetText();
                        }
                    } else if (IsPropertyName(*n, "description")) {
                        if (n->HasText() && !LooksLikeHtml(n->GetText())) {
                            o.Description = n->GetText();
                        }
                    } else if (IsPropertyName(*n, "image")) {
                        if (n->HasHref()) {
                            o.Images.push_back(n->GetHref());
                        }
                    } else if (IsPropertyName(*n, "url")) {
                        if (n->HasHref()) {
                            o.Url = n->GetHref();
                        }
                    }
                }
                if (IsPropertyName(*n, "propertieslist")) {
                    ParsePropertiesList(n, o);
                }
            }
            for (const auto& itemType : v[i]->GetItemtypes()) {
                o.Type = itemType;
                if (o.Type == "product") {
                    ParseProduct(v[i], host, docLang, o);
                }
                if (o.Type.size() && (o.Name.size() || o.Description.size() || !o.Images.empty() || o.Url.size())) {
                    res.push_back(o);
                }
            }
        }
        fineCover = FineSchemaOrgCover(v, guessedSentCount);
    }

    void PickByLen(TVector<const TSchemaObj*>& out, const TVector<const TSchemaObj*>& v,
            size_t maxCnt, size_t maxLen, size_t& cutLen) {
        THashSet<TString> doneObj;
        size_t sum = 0;
        for (size_t i = 0; i < v.size(); ++i) {
            const TString id = v[i]->GetId();
            if (doneObj.find(id) != doneObj.end()) {
                continue;
            }
            sum += Min<size_t>(v[i]->Name.size(), 85);
            sum += v[i]->Description.size();
            doneObj.insert(id);
            out.push_back(v[i]);
            if (out.size() >= maxCnt) {
                break;
            }
        }
        if (sum <= maxLen) {
            cutLen = maxLen;
            return;
        }
        size_t l = 0, r = maxLen;
        TVector<size_t> nameLens;
        TVector<size_t> descLens;
        for (size_t i = 0; i < out.size(); ++i) {
            size_t name = 0;
            size_t descr = 0;
            if (GetNumberOfUTF8Chars(out[i]->Name.data(), out[i]->Name.size(), name)) {
                nameLens.push_back(name);
            }
            if (GetNumberOfUTF8Chars(out[i]->Description.data(), out[i]->Description.size(), descr)) {
                descLens.push_back(descr);
            }
        }
        while (l < r) {
            size_t m = (l + r + 1) / 2;
            size_t s = 0;
            for (size_t i = 0; i < nameLens.size(); ++i) {
                s += Min<size_t>(85, Min(nameLens[i], m));
            }
            for (size_t i = 0; i < descLens.size(); ++i) {
                s += Min(descLens[i], m);
            }
            if (s <= maxLen) {
                l = m;
            } else {
                r = m - 1;
            }
        }
        cutLen = l;
    }

    inline bool IsFromHost(TStringBuf urlHost, TStringBuf host) {
        if (!urlHost.EndsWith(host)) {
            return false;
        }
        return urlHost == host || urlHost[urlHost.size() - host.size() - 1] == '.';
    }

    bool IsListed(const TString& url, const TStringBuf* a, size_t n) {
        TStringBuf host = GetOnlyHost(url);
        for (size_t i = 0; i < n; ++i) {
            if (IsFromHost(host, a[i])) {
                return true;
            }
        }
        return false;
    }

    inline bool IsBlackListed(const TString& url) {
        return IsListed(url, BLACK_LIST, Y_ARRAY_SIZE(BLACK_LIST));
    }

    inline bool IsWhiteListed(const TString& url) {
        return IsListed(url, WHITE_LIST, Y_ARRAY_SIZE(WHITE_LIST));
    }

    bool CheckOgTitle(const TConfig& cfg, const TQueryy& queryCtx, const TUtf16String& ogTitleString, const TUtf16String& natTitleString) {
        TSnipTitle ogTitle(ogTitleString, cfg, queryCtx);
        TSnipTitle natTitle(natTitleString, cfg, queryCtx);
        return GetSimilarity(ogTitle.GetEQInfo(), natTitle.GetEQInfo()) >= 0.9;
    }

    size_t FindMaxLens(const TVector<size_t>& lens, size_t maxLen) {
        const static size_t MAX_POST_LEN = 200;
        size_t l = 0;
        size_t r = 0;
        for (size_t i = 0; i < lens.size(); ++i) {
            r = Max(r, lens[i]);
        }
        r = Min(r, MAX_POST_LEN);
        while (l < r) {
            size_t m = (l + r + 1) / 2;
            size_t totalLen = 0;
            for (size_t i = 0; i < lens.size(); ++i) {
                totalLen += Min(m, lens[i]);
            }
            if (totalLen <= maxLen) {
                l = m;
            } else {
                r = m - 1;
            }
        }
        return l;
    }

    std::pair<int, int> FindMessageStart(const TArchiveView& sents,
            const TContentPreviewViewer& cpViewer) {
        static const std::pair<ui16, ui16> INVALID_PAIR = {1, 0};
        static const size_t MAX_QUOTES_TO_LOOK = 10;
        if (sents.Empty())
            return INVALID_PAIR;

        // find first valid sent
        size_t firstValidSent = 0;
        while (firstValidSent < sents.Size() && sents.Get(firstValidSent) == nullptr)
            ++firstValidSent;
        if (firstValidSent >= sents.Size())
            return INVALID_PAIR;
        // find last valid sent
        size_t lastValidSent = sents.Size() - 1;
        while (firstValidSent <= lastValidSent && sents.Get(lastValidSent) == nullptr)
            --lastValidSent;
        if (firstValidSent > lastValidSent)
            return INVALID_PAIR;
        int postBeg = sents.Get(firstValidSent)->SentId;
        int postEnd = sents.Get(lastValidSent)->SentId;

        // want to return text that is lying not in a quote, among them - its last part
        int resBeg = -1;
        int resEnd = -1; // these two points can belong to different quotes
        const TVector<std::pair<ui16, ui16>>& quoteSpans = cpViewer.GetForumQuoteSpans();
        for (size_t i = 0; i < MAX_QUOTES_TO_LOOK && i < quoteSpans.size(); ++i) {
            int quoteBeg = quoteSpans[i].first;
            if (postBeg <= quoteBeg && quoteBeg <= postEnd)
                resBeg = Max(resBeg, quoteBeg);
            int quoteEnd = quoteSpans[i].second;
            if (postBeg <= quoteEnd && quoteEnd <= postEnd)
                resEnd = Max(resEnd, quoteEnd);
        }

        if (resBeg == -1 && resEnd == -1) { // no quotes in the post - return the whole message
            return {postBeg, postEnd};
        } else if (resBeg == -1) { // strange case
            return {resEnd + 1, postEnd};
        } else if (resEnd == -1) { // another strange case
            return {postBeg, resBeg - 1};
        } else {
            if (resBeg <= resEnd) // the usual case - full quote in the post
                return {resEnd + 1, postEnd};
            else // output something that lies between quotes
                return {resEnd + 1, resBeg - 1};
        }
    }
} // anonymous namespace

    class TPreviewReplacer::TImpl {
    private:
        static const TString TEXT_SRC;

        const TContentPreviewViewer& SentenceViewer;
        const TForumMarkupViewer& ForumsViewer;
        TRetainedSentsMatchInfo Data;
        TSnip Preview;
        TSnipTitle PreviewTitle;
        TString TitleSrc;
        TString PreviewJson;
        THolder<TRawPreviewFiller> RawPreviewFiller;
        TVector<TForumMessageText> ForumMessages;
        int NumAllForumMessages;
        const TContentPreviewViewer& CPViewer;

    private:
        void SuppressCyr(const TReplaceContext& repCtx);
        void FixPreviewBounds(const TSentsInfo& infoBreaks, const TSentsMatchInfo& sentsMatchInfo,
                const TConfig& cfg, int& w0, int& w1);
        TString FilterAndGetTemplate(const TString& url, const TVector<const TSchemaObj*>& vobj, size_t cutLen,
                bool bad, bool veryBad, bool edge, bool badCover);
        void FillData(const TConfig& cfg, const TVector<TSchemaObj>& v, THashSet<TString>& bads,
                THashSet<TString>& veryBads, THashSet<TString>& edges,
                THashMap< TString, TVector<const TSchemaObj*> >& type2obj,
                THashSet<TString>& doneType, TVector<TString>& typesInOrder);
        void FillForumTexts();
        void InitForum(const TReplaceContext& repCtx, const TForumMarkupViewer& forumViewer);
        void InitForumJson(const TString& url);

        std::pair<int, int> GetPreviewBounds(const TSentsInfo& infoBreaks,
                const TSnipTitle& previewTitle) const;
        void FixPreviewBoundsBreaks(const TSentsInfo& infoBreaks, int& w0, int& w1) const;
        int AddProps(const TReplaceContext& repCtx, const TSchemaObj& obj, TPreviewItemFiller& previewItemFiller,
                NJson::TJsonValue& e) const;
        void GetFilteredObjects(const TVector<const TSchemaObj*>& vobj,
                TVector<const TSchemaObj*>& res, size_t& cutLen) const;
        bool IsSimilar(const TSnip& mainSnip, const TSnip& previewSnip) const;
        void FillImages(const TDocInfos& di, TVector<TString>& imgUrls) const;

    public:
        TImpl(const TContentPreviewViewer& sentenceViewer, const TForumMarkupViewer& forums)
          : SentenceViewer(sentenceViewer)
          , ForumsViewer(forums)
          , Data()
          , Preview()
          , PreviewTitle()
          , TitleSrc()
          , PreviewJson()
          , NumAllForumMessages(0)
          , CPViewer(sentenceViewer)
        {
        }

        void Fill(const TReplaceContext& repCtx) {
            RawPreviewFiller.Reset(new TRawPreviewFiller(repCtx.Cfg.NeedFormRawPreview()));
            if (!SpecialInit(repCtx)) {
                if (!InitTitle(repCtx, ForumsViewer, true) && TitleSrc != "natural") {
                    InitTitle(repCtx, ForumsViewer, false);
                }
                if (repCtx.Cfg.ForumForPreview() && SentenceViewer.IsForumDoc()) {
                    InitForum(repCtx, ForumsViewer);
                }
                Init(repCtx, SentenceViewer.GetResult());
            }
            if (SentenceViewer.IsForumDoc()) {
                if (repCtx.Cfg.ForumForPreview()) {
                    InitForumJson(repCtx.Url);
                }
            } else {
                InitJson(repCtx, SentenceViewer);
            }
        }

        bool SpecialInit(const TReplaceContext& repCtx);
        std::pair<TUtf16String, TString> FindSpecialTitle(const TReplaceContext& repCtx, const TForumMarkupViewer& forums);
        bool InitTitle(const TReplaceContext& repCtx, const TForumMarkupViewer& forums, bool allowSpecial);
        void Init(const TReplaceContext& repCtx, const TArchiveView& sentenceView);

        TUtf16String GetTextCopy(const TUtf16String& paramark) const;
        const TSnipTitle& GetPreviewTitle() const {
            return PreviewTitle;
        }
        TString GetTitleSrc() const {
            return TitleSrc;
        }

        void DoWork(TReplaceManager* manager);
        void InitJson(const TReplaceContext& repCtx, const TContentPreviewViewer& viewer);
        void DoBasicJson(const TReplaceContext& repCtx, const TContentPreviewViewer& viewer);
        void DoSchemaJson(const TReplaceContext& repCtx, const NSchemaOrg::TTreeNode* schema, const THashSet<int>& mainContent,
                const THashSet<int>& goodContent, int guessedSentCount);
        TUtf16String PreviewCut(const TReplaceContext& repCtx, const TUtf16String& s, size_t len, bool forceDots) const;
        TString PreviewCut(const TReplaceContext& repCtx, const TString& s, size_t len, bool forceDots) const;
        bool DoOne(const TReplaceContext& repCtx, NJson::TJsonValue& res, TRawPreviewFiller& rawPreviewFillerRes,
                const TSchemaObj& obj, bool bad);
        bool DoMany(const TReplaceContext& repCtx, NJson::TJsonValue& res, TRawPreviewFiller& rawPreviewFillerRes,
                const TVector<const TSchemaObj*>& vobj, bool bad, bool veryBad, bool edge,
                bool badCover, const TString& url);
    };

    const TString TPreviewReplacer::TImpl::TEXT_SRC = "content_preview";

    void TPreviewReplacer::TImpl::SuppressCyr(const TReplaceContext& repCtx) {
        if (repCtx.Cfg.SuppressCyrForTr() && !repCtx.QueryCtx.CyrillicQuery) {
            if (HasTooManyCyrillicWords(Preview.GetRawTextWithEllipsis(), 2)) {
                Preview = TSnip();
            }
            if (HasTooManyCyrillicWords(PreviewTitle.GetTitleString(), 1)) {
                PreviewTitle = TSnipTitle();
            }
        }
    }

    std::pair<int, int> TPreviewReplacer::TImpl::GetPreviewBounds(const TSentsInfo& infoBreaks,
            const TSnipTitle& previewTitle) const {
        THashSet<size_t> titleWords;
        if (previewTitle.GetSentsInfo()) {
            for (const auto& word : previewTitle.GetSentsInfo()->WordVal) {
                titleWords.insert(word.Word.Hash);
            }
        }

        int w0 = 0;
        int w1 = infoBreaks.WordCount() - 1;
        while (w1 >= w0) {
            int s0 = infoBreaks.WordId2SentId(w0);
            int w01 = infoBreaks.LastWordIdInSent(s0);
            bool good = false;
            for (int i = w0; i <= w01; ++i) {
                if (!titleWords.contains(infoBreaks.WordVal[i].Word.Hash)) {
                    good = true;
                    break;
                }
            }
            if (good) {
                break;
            }
            w0 = w01 + 1;
        }
        return {w0, w1};
    }

    static bool IsGoodSentBreak(const TWtringBuf& punctAfter) {
        const TUtf16String GOOD_BREAK_PUNCT = u",;()";
        return punctAfter.find_first_of(GOOD_BREAK_PUNCT) != punctAfter.npos;
    }

    // TODO: join with FixPreviewBoundsBreaks
    static int PreviewSmartCut(int w0, int w1, const TSentsMatchInfo& sentsMatchInfo, float maxLen) {
        TWordSpanLen wordSpanLen(TCutParams::Symbol());

        const int lastFit = wordSpanLen.FindFirstWordLonger(sentsMatchInfo, w0, w1, maxLen) - 1;
        if (lastFit < w0) {
            return -1;
        }

        const float minLen = maxLen * (2.f / 3.f);
        const int firstFit = wordSpanLen.FindFirstWordLonger(sentsMatchInfo, w0, lastFit, minLen);

        const TSentsInfo& sentsInfo = sentsMatchInfo.SentsInfo;
        for (int word = lastFit; word >= firstFit; --word) {
            if (sentsInfo.IsWordIdLastInSent(word)) {
                return word;
            }
        }
        for (int word = lastFit; word > firstFit; --word) {
            if (IsGoodSentBreak(sentsInfo.GetBlanksAfter(word))) {
                return word;
            }
        }
        return lastFit;
    }

    void TPreviewReplacer::TImpl::FixPreviewBounds(const TSentsInfo& infoBreaks,
            const TSentsMatchInfo& sentsMatchInfo, const TConfig& cfg, int& w0, int& w1) {
        int s0 = infoBreaks.WordId2SentId(w0);
        int s1 = infoBreaks.WordId2SentId(w1);
        while (s0 < s1) {
            int l = infoBreaks.GetSentBuf(s0).size();
            int i = s0 + 1;
            while (i <= s1) {
                if (infoBreaks.IsSentIdFirstInPara(i)) {
                    break;
                }
                l += infoBreaks.GetSentBuf(i).size();
                ++i;
            }
            if (l > 7) {
                break;
            }
            s0 = i;
            if (s0 > s1) {
                return;
            }
            w0 = infoBreaks.FirstWordIdInSent(s0);
        }
        w1 = PreviewSmartCut(w0, w1, sentsMatchInfo, cfg.FStannLen());
    }

    void TPreviewReplacer::TImpl::FixPreviewBoundsBreaks(const TSentsInfo& infoBreaks, int& w0,
            int& w1) const {
        int s0 = infoBreaks.WordId2SentId(w0);
        int s1 = infoBreaks.WordId2SentId(w1);
        int npara = 1;
        for (int i = s0 + 1; i <= s1; ++i) {
            if (infoBreaks.IsSentIdFirstInPara(i)) {
                if (npara == 10) {
                    s1 = i - 1;
                    w1 = infoBreaks.LastWordIdInSent(s1);
                    break;
                }
                ++npara;
            }
        }
        if (s1 > s0 && infoBreaks.IsSentIdFirstInPara(s1)) {
            int w10 = infoBreaks.FirstWordIdInSent(s1);
            if (w1 - w10 + 1 <= 5) {
                if (!infoBreaks.IsSentIdFirstInPara(s1 - 1) || infoBreaks.GetSentLengthInWords(s1 - 1) > 5) {
                    w1 = w10 - 1;
                }
            }
        }
    }

    int TPreviewReplacer::TImpl::AddProps(const TReplaceContext& repCtx, const TSchemaObj& obj,
            TPreviewItemFiller& previewItemFiller, NJson::TJsonValue& e) const {
        const int MAX_PROPS = 5;
        int totalProps = 0;
        if (obj.Price.Currency) {
            NJson::TJsonValue price(NJson::JSON_MAP);
            if (obj.Price.PriceMin && obj.Price.PriceMinCent) {
                price.InsertValue("priceMin", obj.Price.PriceMin);
                price.InsertValue("priceMinCent", obj.Price.PriceMinCent);
            }
            if (obj.Price.PriceMax && obj.Price.PriceMaxCent) {
                price.InsertValue("priceMax", obj.Price.PriceMax);
                price.InsertValue("priceMaxCent", obj.Price.PriceMaxCent);
            }
            price.InsertValue("currency", obj.Price.Currency);
            e.InsertValue("price", price);
            ++totalProps;
        }
        if (obj.Rating.RatingValue && obj.Rating.RatingBest) {
            NJson::TJsonValue rating(NJson::JSON_MAP);
            rating.InsertValue("rating_value", obj.Rating.RatingValue);
            rating.InsertValue("best_rating", obj.Rating.RatingBest);
            if (obj.Rating.RatingCount) {
                rating.InsertValue("rating_count", obj.Rating.RatingCount);
            }
            e.InsertValue("rating", rating);
        }
        if (!obj.Reviews.empty()) {
            if (obj.ReviewCount) {
                e.InsertValue("reviews_count", obj.ReviewCount);
            }
            NJson::TJsonValue reviews(NJson::JSON_ARRAY);
            for (const auto& objReview : obj.Reviews) {
                NJson::TJsonValue review(NJson::JSON_MAP);
                NJson::TJsonValue passages(NJson::JSON_ARRAY);
                TString reviewBody = PreviewCut(repCtx, objReview.ReviewBody, 200, false);
                passages.AppendValue(reviewBody);
                review.InsertValue("passages", passages);
                if (objReview.DatePublished) {
                    review.InsertValue("date", objReview.DatePublished);
                }
                if (objReview.ReviewAuthor) {
                    TString author = PreviewCut(repCtx, objReview.ReviewAuthor, 32, false);
                    review.InsertValue("author", author);
                }
                reviews.AppendValue(review);
            }
            e.InsertValue("reviews", reviews);
        }
        if (obj.MainProps.size() > 0) {
            NJson::TJsonValue props(NJson::JSON_ARRAY);
            for (size_t i = 0; totalProps < MAX_PROPS && i < obj.MainProps.size(); ++i) {
                NJson::TJsonValue prop(NJson::JSON_ARRAY);
                prop.AppendValue(obj.MainProps[i].first);
                prop.AppendValue(obj.MainProps[i].second);
                props.AppendValue(prop);
                ++totalProps;
                previewItemFiller.AddProperty(obj.MainProps[i].first, obj.MainProps[i].second, true);
            }
            e.InsertValue("main-properties", props);
        }
        if (obj.Props.size() > 0 && totalProps < MAX_PROPS) {
            NJson::TJsonValue props(NJson::JSON_ARRAY);
            for (size_t i = 0; totalProps < MAX_PROPS && i < obj.Props.size(); ++i) {
                NJson::TJsonValue prop(NJson::JSON_ARRAY);
                prop.AppendValue(obj.Props[i].first);
                prop.AppendValue(obj.Props[i].second);
                props.AppendValue(prop);
                ++totalProps;
                previewItemFiller.AddProperty(obj.Props[i].first, obj.Props[i].second, false);
            }
            e.InsertValue("properties", props);
        }

        return totalProps;
    }

    void TPreviewReplacer::TImpl::GetFilteredObjects(const TVector<const TSchemaObj*>& vobj,
            TVector<const TSchemaObj*>& res, size_t& cutLen) const {
        TVector<const TSchemaObj*> vFilteredObj;
        for (size_t i = 0; i < vobj.size(); ++i) {
            if (vobj[i]->Name.size() && SameNameDescr(vobj[i]->Name, vobj[i]->Description) &&
                    !vobj[i]->Props.size() && !vobj[i]->MainProps.size()) {
                continue;
            }
            vFilteredObj.push_back(vobj[i]);
        }
        PickByLen(res, vFilteredObj, 6, 835, cutLen);
    }

    TString TPreviewReplacer::TImpl::FilterAndGetTemplate(const TString& url, const TVector<const TSchemaObj*>& vobj,
            size_t cutLen, bool bad, bool veryBad, bool edge, bool badCover) {
        TString resTmpl;
        bool allimg = true;
        bool allDescr = true;
        bool allName = true;
        bool hasLongDescr = false;
        for (size_t i = 0; i < vobj.size(); ++i) {
            if (vobj[i]->Name.size() < 5) {
                allName = false;
            }
            if (vobj[i]->Description.size()) {
                if (Min(vobj[i]->Description.size(), cutLen) >= 20) {
                    hasLongDescr = true;
                }
            } else {
                allDescr = false;
            }
            if (vobj[i]->Images.size() == 0) {
                allimg = false;
            }
        }

        bool catalog = false;
        bool catalogImg = false;
        if (allimg && allName) {
            resTmpl = "catalog-images";
            catalogImg = true;
        } else if ((hasLongDescr && allName && allDescr)) {
            resTmpl = "catalog";
            catalog = true;
        } else {
            return TString();
        }
        bool white = IsWhiteListed(url);
        if (bad && !((catalog || catalogImg) && (!veryBad || white))) {
            return TString();
        }

        if ((catalog || catalogImg) && (edge || badCover) && !white) {
            return TString();
        }

        return resTmpl;
    }

    void TPreviewReplacer::TImpl::FillData(const TConfig& cfg, const TVector<TSchemaObj>& v, THashSet<TString>& bads,
            THashSet<TString>& veryBads, THashSet<TString>& edges,
            THashMap< TString, TVector<const TSchemaObj*> >& type2obj,
            THashSet<TString>& doneType, TVector<TString>& typesInOrder) {
        for (size_t i = 0; i < v.size(); ++i) {
            if (doneType.find(v[i].Type) == doneType.end()) {
                doneType.insert(v[i].Type);
                typesInOrder.push_back(v[i].Type);
            }
            type2obj[v[i].Type].push_back(&v[i]);
            if (!cfg.SchemaPreviewSegmentsOff()) {
                if (v[i].BadContent) {
                    bads.insert(v[i].Type);
                }
                if (v[i].VeryBadContent) {
                    veryBads.insert(v[i].Type);
                }
                if (v[i].EdgeContent) {
                    edges.insert(v[i].Type);
                }
            }
        }
    }

    void TPreviewReplacer::TImpl::FillForumTexts() {
        const static size_t MAX_FORUM_LEN = 835;
        if (ForumMessages.empty()) {
            return;
        }

        TVector<size_t> lens(ForumMessages.size());
        for (size_t i = 0; i < ForumMessages.size(); ++i) {
            lens[i] = ForumMessages[i].GetSentsInfo()->Text.size();
        }
        const size_t maxPostLen = FindMaxLens(lens, MAX_FORUM_LEN);
        for (size_t i = 0; i < lens.size(); ++i) {
            lens[i] = Min(lens[i], maxPostLen);
        }

        TVector<TEQInfo> eqInfos;
        for (size_t i = 0; i < ForumMessages.size(); ++i) {
            int lastWord = ForumMessages[i].GetSentsMatchInfo()->WordsCount() - 1;
            if (lens[i] < ForumMessages[i].GetSentsInfo()->Text.size()) { // we have to cut this post
                lastWord = PreviewSmartCut(0, lastWord, *ForumMessages[i].GetSentsMatchInfo(), lens[i]);
                if (lastWord < 0) {
                    continue;
                }
            }

            // Check for similarity with the previous posts
            TEQInfo eqInfo(*ForumMessages[i].GetSentsMatchInfo(), 0, lastWord);
            bool looksLikeQuote = false;
            for (size_t j = 0; j < eqInfos.size(); ++j) {
                if (GetSimilarity(eqInfo, eqInfos[j]) > 0.85) {
                    looksLikeQuote = true;
                    break;
                }
            }
            if (!looksLikeQuote) {
                ForumMessages[i].FinalText =
                    ForumMessages[i].GetSentsInfo()->GetTextWithEllipsis(0, lastWord);
                eqInfos.push_back(eqInfo);
            }
        }
    }

    void TPreviewReplacer::TImpl::InitForum(const TReplaceContext& repCtx, const TForumMarkupViewer& forumViewer) {
        static const size_t MAX_POSTS_IN_CP = 7;
        static const size_t MAX_POSTS_TO_LOOK = 100; // for performance reasons
        if (!forumViewer.FilterByMessages()) {
            return;
        }
        const TForumMarkupViewer::TZones& fZones = forumViewer.ForumZones;
        TVector<TForumMessageHelper> tmpV;
        for (size_t i = 0; i < fZones.size() && i < MAX_POSTS_TO_LOOK; ++i) {
            const TForumMessageZone& zone = forumViewer.ForumZones[i];
            TForumMessageHelper messageHelper;
            messageHelper.Zone = &zone;
            messageHelper.Date = GetDateFromString(zone.Date);
            tmpV.push_back(messageHelper);
        }
        if (tmpV.empty()) {
            return;
        }
        Sort(tmpV.begin(), tmpV.end()); // sort by date, then by zone
        for (size_t i = 0; i < tmpV.size() && ForumMessages.size() < MAX_POSTS_IN_CP; ++i) {
            const TForumMessageZone& zone = *tmpV[i].Zone;
            if (zone.Sents.Empty()) {
                continue;
            }
            std::pair<int, int> goodSentsInterval = FindMessageStart(zone.Sents, CPViewer); // exlude quote from post
            if (goodSentsInterval.first > goodSentsInterval.second) { // invalid span
                continue;
            }
            TArchiveView view;
            for (size_t j = 0; j < zone.Sents.Size(); ++j) {
                if (zone.Sents.Get(j) != nullptr && goodSentsInterval.first <= zone.Sents.Get(j)->SentId &&
                        zone.Sents.Get(j)->SentId <= goodSentsInterval.second) {
                    view.PushBack(zone.Sents.Get(j));
                }
            }
            TForumMessageText message;
            message.SetView(&repCtx.Markup, view, TRetainedSentsMatchInfo::TParams(repCtx.Cfg, repCtx.QueryCtx));
            if (message.GetSentsInfo()->WordCount() == 0) {
                continue;
            }
            message.Zone = &zone;
            message.Date = GetDateFromString(zone.Date);
            ForumMessages.push_back(message);
        }
        FillForumTexts();
        NumAllForumMessages = forumViewer.NumItems;
    }

    void TPreviewReplacer::TImpl::InitForumJson(const TString& url) {
        if (ForumMessages.empty()) {
            return;
        }

        // We don't fill RawPreviewFiller since it seems to be unnecessary
        // So for now it will NOT work in NeedFormRawPreview mode
        NJson::TJsonValue answer(NJson::JSON_MAP);
        answer.InsertValue("template", "object");
        answer.InsertValue("results_count", 1);
        NJson::TJsonValue forum(NJson::JSON_MAP);
        TUtf16String title = PreviewTitle.GetTitleString();
        if (title.size()) {
            TString name = WideToUTF8(title);
            forum.InsertValue("name", name);
        }
        if (NumAllForumMessages > 0) {
            forum.InsertValue("posts_count", NumAllForumMessages);
        }

        NJson::TJsonValue posts(NJson::JSON_ARRAY);
        for (size_t i = 0; i < ForumMessages.size(); ++i) {
            if (ForumMessages[i].FinalText.empty()) {
                continue;
            }
            NJson::TJsonValue post(NJson::JSON_MAP);
            if (ForumMessages[i].Zone->Date.size())
                post.InsertValue("date", WideToUTF8(ForumMessages[i].Zone->Date));
            if (ForumMessages[i].Zone->Author.size())
                post.InsertValue("author", WideToUTF8(ForumMessages[i].Zone->Author));
            if (ForumMessages[i].Zone->Anchor.size())
                post.InsertValue("url", AddSchemePrefix(url + WideToUTF8(ForumMessages[i].Zone->Anchor)));

            NJson::TJsonValue passages(NJson::JSON_ARRAY);
            passages.AppendValue(WideToUTF8(ForumMessages[i].FinalText));
            post.InsertValue("passages", passages);
            posts.AppendValue(post);
        }
        Y_ASSERT(posts.GetArray().size() > 0);
        forum.InsertValue("posts", posts);
        NJson::TJsonValue object(NJson::JSON_ARRAY);
        object.AppendValue(forum);
        answer.InsertValue("results", object);

        // save the answer
        TStringOutput o(PreviewJson);
        NJson::WriteJson(&o, &answer, true);
    }

    bool TPreviewReplacer::TImpl::IsSimilar(const TSnip& mainSnip,
            const TSnip& previewSnip) const {
        if (mainSnip.Snips && previewSnip.Snips) {
            return GetSimilarity(TEQInfo(mainSnip), TEQInfo(previewSnip), true) >= 0.65;
        }
        return false;
    }

    void TPreviewReplacer::TImpl::FillImages(const TDocInfos& di, TVector<TString>& imgUrls) const {
        if (di.find("imagessnip") != di.end()) {
            TString s = di.find("imagessnip")->second;
            NJson::TJsonValue v;
            TMemoryInput mi(s.data(), s.size());
            if (NJson::ReadJsonTree(&mi, &v)) {
                const NJson::TJsonValue::TMapType* m = nullptr;
                const NJson::TJsonValue::TArray* a = nullptr;
                if (v.GetMapPointer(&m) && m->find("Images") != m->end() && m->find("Images")->second.GetArrayPointer(&a)) {
                    for (size_t i = 0; i < 6 && i < a->size(); ++i) {
                        const NJson::TJsonValue::TMapType* ma = nullptr;
                        TString u;
                        if ((*a)[i].GetMapPointer(&ma) && ma->find("u") != ma->end() && ma->find("u")->second.GetString(&u)) {
                            imgUrls.push_back(AddSchemePrefix(u));
                        }
                    }
                }
            }
        }
    }

    void TPreviewReplacer::TImpl::InitJson(const TReplaceContext& repCtx, const TContentPreviewViewer& viewer) {
        TDocInfos::const_iterator di = repCtx.DocInfos.find("ya_preview_disallow");
        if (di != repCtx.DocInfos.end() && di->second == TStringBuf("all")) {
            return;
        }
        if (repCtx.Cfg.DisallowVthumbPreview()) {
            di = repCtx.DocInfos.find("vthumb");
            if (di != repCtx.DocInfos.end()) {
                return;
            }
        }
        if (repCtx.Cfg.SchemaPreview() && viewer.GetSchema()) {
            DoSchemaJson(repCtx, viewer.GetSchema(), viewer.GetMainContent(), viewer.GetGoodContent(),
                    viewer.GuessSentCount());
        }
        if (!PreviewJson.size()) {
            DoBasicJson(repCtx, viewer);
        }
        if (!PreviewJson.size()) {
            const auto& additionalResults = viewer.GetAdditionalResults();
            for (auto it = additionalResults.begin(); it != additionalResults.end() && !PreviewJson.size(); ++it) {
                Init(repCtx, *it);
                DoBasicJson(repCtx, viewer);
            }
        }
    }

    TUtf16String TPreviewReplacer::TImpl::PreviewCut(const TReplaceContext& repCtx, const TUtf16String& s, size_t len, bool forceDots) const {
        TUtf16String w = s;
        TInlineHighlighter ih;
        ih.PaintPassages(w);
        if (w.size() <= len && !forceDots) {
            return w;
        }
        TSmartCutOptions options(repCtx.Cfg);
        options.MaximizeLen = true;
        options.CutParams = TCutParams::Symbol();
        SmartCut(w, repCtx.IH, len, options);
        return w;
    }

    TString TPreviewReplacer::TImpl::PreviewCut(const TReplaceContext& repCtx, const TString& s, size_t len, bool forceDots) const {
        return WideToUTF8(PreviewCut(repCtx, UTF8ToWide(s), len, forceDots));
    }

    bool TPreviewReplacer::TImpl::DoOne(const TReplaceContext& repCtx, NJson::TJsonValue& res,
            TRawPreviewFiller& rawPreviewFillerRes, const TSchemaObj& obj, bool bad) {
        size_t cutLen = 0;
        TVector<const TSchemaObj*> vobj(1, &obj);
        TVector<const TSchemaObj*> v;
        PickByLen(v, vobj, 1, 835, cutLen);
        NJson::TJsonValue mres(NJson::JSON_MAP);
        TRawPreviewFiller tempRawPreviewFillerRes(repCtx.Cfg.NeedFormRawPreview());
        mres.InsertValue("results_count", 1);
        tempRawPreviewFillerRes.SetResultsCountInAll(1);
        TPreviewItemFiller previewItemFiller = tempRawPreviewFillerRes.GetPreviewItemFiller();
        NJson::TJsonValue e(NJson::JSON_MAP);

        if (obj.Name.size()) {
            TString name = PreviewCut(repCtx, obj.Name, Min<size_t>(cutLen, 85), false);
            e.InsertValue("name", name);
            previewItemFiller.SetName(name);
        }

        TString description = MakeDoubleBR(PreviewCut(repCtx, obj.Description, cutLen, true));
        e.InsertValue("description", description);
        previewItemFiller.SetDescription(description);

        TString tpl;
        bool imgsOut = false;
        if (obj.Images.size() > 1) {
            NJson::TJsonValue imgs(NJson::JSON_ARRAY);
            for (size_t i = 0; i < obj.Images.size() && i < 5; ++i) {
                imgs.AppendValue(obj.Images[i]);
                previewItemFiller.AddImage(obj.Images[i]);
            }
            tpl = "article-images";
            e.InsertValue("images", imgs);
            imgsOut = true;
        } else {
            tpl = "article";
            if (obj.Images.size() == 1) {
                e.InsertValue("image", obj.Images[0]);
                previewItemFiller.AddImage(obj.Images[0]);
                imgsOut = true;
            }
        }

        int totalProps = AddProps(repCtx, obj, previewItemFiller, e);
        if (totalProps > 0) {
            tpl = "object";
        }

        if (tpl == "object" && obj.Description.size() < 100 && !imgsOut) {
            return false;
        }
        if (bad && tpl != "object") {
            return false;
        }
        if ((Min(obj.Description.size(), cutLen) < 250 || !obj.Name.size() || SameNameDescr(obj.Name, obj.Description)) && tpl != "object") {
            return false;
        }

        // saving last parts
        mres.InsertValue("template", tpl);
        tempRawPreviewFillerRes.SetTemplate(tpl);
        NJson::TJsonValue a(NJson::JSON_ARRAY);
        a.AppendValue(e);
        mres.InsertValue("results", a);

        // saving total result
        res = mres;
        rawPreviewFillerRes.SetRawPreview(tempRawPreviewFillerRes);
        return true;
    }

    bool TPreviewReplacer::TImpl::DoMany(const TReplaceContext& repCtx, NJson::TJsonValue& res,
            TRawPreviewFiller& rawPreviewFillerRes, const TVector<const TSchemaObj*>& vobj,
            bool bad, bool veryBad, bool edge, bool badCover, const TString& url) {
        if (IsBlackListed(url)) {
            return false;
        }

        TVector<const TSchemaObj*> v;
        size_t cutLen = 0;
        GetFilteredObjects(vobj, v, cutLen);
        if (v.size() < 3) {
            return false;
        }

        TString tpl = FilterAndGetTemplate(repCtx.Url, vobj, cutLen, bad, veryBad, edge, badCover);
        if (tpl.empty()) {
            return false;
        }

        NJson::TJsonValue mres(NJson::JSON_MAP);
        TRawPreviewFiller tempRawPreviewFillerRes(repCtx.Cfg.NeedFormRawPreview());

        NJson::TJsonValue a(NJson::JSON_ARRAY);
        for (size_t i = 0; i < v.size(); ++i) {
            NJson::TJsonValue e(NJson::JSON_MAP);
            TPreviewItemFiller previewItemFiller = tempRawPreviewFillerRes.GetPreviewItemFiller();
            if (v[i]->Name.size()) {
                TString name = PreviewCut(repCtx, v[i]->Name, Min<size_t>(cutLen, 85), false);
                e.InsertValue("name", name);
                previewItemFiller.SetName(name);
            }
            if (v[i]->Description.size()) {
                TString description = MakeDoubleBR(PreviewCut(repCtx, v[i]->Description, cutLen, true));
                e.InsertValue("description", description);
                previewItemFiller.SetDescription(description);
            }
            if (v[i]->Url.size()) {
                e.InsertValue("url", v[i]->Url);
                previewItemFiller.SetUrl(v[i]->Url);
            }
            if (v[i]->Images.size() > 0) {
                e.InsertValue("image", v[i]->Images[0]);
                previewItemFiller.AddImage(v[i]->Images[0]);
            }
            AddProps(repCtx, *v[i], previewItemFiller, e);
            a.AppendValue(e);
        }

        mres.InsertValue("template", tpl);
        tempRawPreviewFillerRes.SetTemplate(tpl);

        mres.InsertValue("results", a);
        mres.InsertValue("results_count", vobj.size());
        tempRawPreviewFillerRes.SetResultsCountInAll(vobj.size());
        res = mres;
        rawPreviewFillerRes.SetRawPreview(tempRawPreviewFillerRes);
        return true;
    }

    void TPreviewReplacer::TImpl::DoSchemaJson(const TReplaceContext& repCtx, const NSchemaOrg::TTreeNode* root,
            const THashSet<int>& mainContent, const THashSet<int>& goodContent,
            int guessedSentCount) {
        TVector<TSchemaObj> v;
        bool fineCover = false;
        ParseObjs(v, root, mainContent, goodContent, guessedSentCount, fineCover,
            GetOnlyHost(repCtx.Url), repCtx.DocLangId);
        if (repCtx.Cfg.SchemaPreviewSegmentsOff()) {
            fineCover = true;
        }
        THashSet<TString> bads;
        THashSet<TString> veryBads;
        THashSet<TString> edges;
        THashMap< TString, TVector<const TSchemaObj*> > type2obj;
        THashSet<TString> doneType;
        TVector<TString> typesInOrder;
        FillData(repCtx.Cfg, v, bads, veryBads, edges, type2obj, doneType, typesInOrder);

        bool done = false;
        NJson::TJsonValue res(NJson::JSON_MAP);
        TRawPreviewFiller rawPreviewFillerRes(repCtx.Cfg.NeedFormRawPreview());
        for (int t = 0; !done && t < 2; ++t) {
            for (size_t i = 0; i < typesInOrder.size(); ++i) {
                const THashMap< TString, TVector<const TSchemaObj*> >::const_iterator it = type2obj.find(typesInOrder[i]);
                if (it == type2obj.end()) {
                    Y_ASSERT(false);
                    continue;
                }
                bool bad = bads.find(it->first) != bads.end();
                bool veryBad = veryBads.find(it->first) != veryBads.end();
                bool edge = edges.find(it->first) != edges.end();
                if (t == 0 && it->second.size() > 1 &&
                        DoMany(repCtx, res, rawPreviewFillerRes, it->second, bad, veryBad, edge,
                            !fineCover, repCtx.Url)) {
                    done = true;
                    break;
                }
                if (t > 0 && it->second.size() == 1 &&
                        DoOne(repCtx, res, rawPreviewFillerRes, *it->second[0], bad)) {
                    done = true;
                    break;
                }
            }
        }
        if (!done) {
            return;
        }
        TString s;
        TStringOutput o(s);
        NJson::TJsonWriter w(&o, true);
        w.Write(&res);
        w.Flush();
        PreviewJson = s;
        RawPreviewFiller->SetRawPreview(rawPreviewFillerRes);
    }

    void TPreviewReplacer::TImpl::DoBasicJson(const TReplaceContext& repCtx, const TContentPreviewViewer& viewer) {
        if (viewer.GuessSentCount() * 4 > viewer.GetContentSentCount() * 10) {
            return;
        }
        if (IsSimilar(repCtx.Snip, Preview)) {
            return;
        }

        TVector<TString> imgUrls;
        FillImages(repCtx.DocInfos, imgUrls);
        bool moreThan3Imgs = imgUrls.size() >= 3;
        bool imgArticle = moreThan3Imgs && repCtx.Cfg.PreviewBaseImagesHack();

        NJson::TJsonValue res(NJson::JSON_MAP);
        TRawPreviewFiller rawPreviewFiller(repCtx.Cfg.NeedFormRawPreview());
        TString tpl;
        if (imgArticle) {
            tpl = "article-images";
            res.InsertValue("results_count", 1);
            rawPreviewFiller.SetResultsCountInAll(1);
        } else if (moreThan3Imgs) {
            if (repCtx.Cfg.DropBaseImagesPreview()) {
                return;
            } else if (repCtx.Cfg.MaskBaseImagesPreview()) {
                tpl = "base";
            } else {
                tpl = "base-images";
            }
        } else {
            tpl = "base";
        }
        res.InsertValue("template", tpl);
        rawPreviewFiller.SetTemplate(tpl);

        TUtf16String headline = GetTextCopy(TUtf16String(1, '\n'));
        headline = PreviewCut(repCtx, headline, headline.size() + 1, false);
        NJson::TJsonValue* place = &res;
        NJson::TJsonValue e(NJson::JSON_MAP);
        if (imgArticle) {
            place = &e;
        }
        TPreviewItemFiller previewItemFiller = rawPreviewFiller.GetPreviewItemFiller();
        bool someHeadline = false;
        if (headline.size() > 250) {
            someHeadline = true;
            TString description = MakeDoubleBR(WideToUTF8(headline));
            place->InsertValue("description", description);
            previewItemFiller.SetDescription(description);
        }
        if (!someHeadline) {
            return;
        }

        TUtf16String title = PreviewTitle.GetTitleString();
        if (title.size()) {
            TString name = WideToUTF8(title);
            place->InsertValue("name", name);
            previewItemFiller.SetName(name);
        }

        if (imgArticle || repCtx.Cfg.PreviewBaseImagesHackV2()) {
            NJson::TJsonValue imgs(NJson::JSON_ARRAY);
            for (size_t i = 0; i < imgUrls.size(); ++i) {
                imgs.AppendValue(imgUrls[i]);
                previewItemFiller.AddImage(imgUrls[i]);
            }
            place->InsertValue("images", imgs);
        }
        if (imgArticle) {
            NJson::TJsonValue a(NJson::JSON_ARRAY);
            a.AppendValue(*place);
            res.InsertValue("results", a);
        }

        // save answer
        TStringOutput o(PreviewJson);
        NJson::TJsonWriter w(&o, true);
        w.Write(&res);
        w.Flush();
        RawPreviewFiller->SetRawPreview(rawPreviewFiller);
        return;
    }

    bool TPreviewReplacer::TImpl::SpecialInit(const TReplaceContext& repCtx) {
        TStaticData data(repCtx.DocInfos, "mediawiki");
        const TUtf16String& title = data.Attrs["title"];
        TUtf16String desc = data.Attrs["desc"];
        if (!title || !desc) {
            return false;
        }
        PreviewTitle = TSnipTitle(title, repCtx.Cfg, repCtx.QueryCtx);
        TitleSrc = "preview_attr";
        SubstGlobal(desc, MEDIA_WIKI_PARA_MARKER, TUtf16String(wchar16('\n')));
        TSmartCutOptions options(repCtx.Cfg);
        options.CutParams = TCutParams::Symbol();
        SmartCut(desc, repCtx.IH, repCtx.Cfg.FStannLen(), options);
        Data.SetView(desc, TRetainedSentsMatchInfo::TParams(repCtx.Cfg, repCtx.QueryCtx));
        const TSentsMatchInfo& smi = *Data.GetSentsMatchInfo();
        if (!smi.WordsCount()) {
            return false;
        }
        Preview = TSnip(TSingleSnip(0, smi.WordsCount() - 1, smi), InvalidWeight);
        return true;
    }

    std::pair<TUtf16String, TString> TPreviewReplacer::TImpl::FindSpecialTitle(const TReplaceContext& repCtx, const TForumMarkupViewer& forums) {
        TUtf16String newsTitle = TStaticData(repCtx.DocInfos, "news").Attrs["title"];
        if (newsTitle) {
            return {newsTitle, "news"};
        }
        TUtf16String marketTitle = TStaticData(repCtx.DocInfos, "market").Attrs["title"];
        if (marketTitle) {
            return {marketTitle, "market"};
        }
        if (forums.ForumTitle) {
            return {forums.ForumTitle, "forums"};
        }
        TUtf16String ogTitle = TOgTitleData(repCtx.DocInfos).Title;
        if (ogTitle && CheckOgTitle(repCtx.Cfg, repCtx.QueryCtx, ogTitle, repCtx.NaturalTitleSource)) {
            return {ogTitle, "ogtitle"};
        }
        return {TUtf16String(), TString()};
    }

    bool TPreviewReplacer::TImpl::InitTitle(const TReplaceContext& repCtx, const TForumMarkupViewer& forums, bool allowSpecial) {
        TUtf16String titleSource = repCtx.NaturalTitleSource;
        TitleSrc = "natural";
        if (allowSpecial) {
            std::pair<TUtf16String, TString> specialTitle = FindSpecialTitle(repCtx, forums);
            if (specialTitle.first) {
                titleSource = specialTitle.first;
                TitleSrc = specialTitle.second;
            }
        }

        // Drop url-like titles
        if (LooksLikeUrl(titleSource)) {
            return false;
        }

        TSmartCutOptions options(repCtx.Cfg);
        options.CutParams = TCutParams::Pixel(repCtx.Cfg.FStannTitleLen(), repCtx.Cfg.GetSnipFontSize());
        options.Threshold = 6.f / 7.f;
        options.AddEllipsisToShortText = false;
        SmartCut(titleSource, repCtx.IH, 2.f, options);

        Strip(titleSource);
        if (titleSource.length() <= 1) {
            return false;
        }

        // Drop last '.' for unification
        if (titleSource && !titleSource.EndsWith(BOUNDARY_ELLIPSIS) && titleSource.back() == '.') {
            titleSource.pop_back();
        }

        PreviewTitle = TSnipTitle(titleSource, repCtx.Cfg, repCtx.QueryCtx);
        return true;
    }

    void TPreviewReplacer::TImpl::Init(const TReplaceContext& repCtx, const TArchiveView& sentenceView) {
        Preview = TSnip();

        int maxLen = repCtx.Cfg.FStannLen();
        TArchiveView fview = GetFirstSentences(sentenceView, maxLen);

        if (fview.Empty()) {
            return;
        }

        Data.SetView(&repCtx.Markup, fview, TRetainedSentsMatchInfo::TParams(repCtx.Cfg, repCtx.QueryCtx).SetPutDot().SetParaTables());
        const TSentsInfo& infoBreaks = *Data.GetSentsInfo();
        const TSentsMatchInfo& sentsMatchInfo = *Data.GetSentsMatchInfo();

        if (!infoBreaks.WordCount()) {
            return;
        }

        std::pair<int, int> previewBounds = GetPreviewBounds(infoBreaks, PreviewTitle);
        int w0 = previewBounds.first;
        int w1 = previewBounds.second;
        if (w1 < w0) {
            return;
        }

        FixPreviewBounds(infoBreaks, sentsMatchInfo, repCtx.Cfg, w0, w1);
        if (w1 < w0) {
            return;
        }

        FixPreviewBoundsBreaks(infoBreaks, w0, w1);
        if (w1 < w0) {
            return;
        }

        TWordSpanLen wordSpanLen(TCutParams::Symbol());
        if (wordSpanLen.CalcLength(TSingleSnip(w0, w1, sentsMatchInfo)) < 64) {
            return;
        }
        Preview = TSnip(TSingleSnip(w0, w1, sentsMatchInfo), InvalidWeight);
        SuppressCyr(repCtx); //suppress cyr on .tr
    }

    TUtf16String TPreviewReplacer::TImpl::GetTextCopy(const TUtf16String& paramark) const {
        if (Preview.Snips.empty()) {
            return TUtf16String();
        }
        TGluer g(&*Preview.Snips.begin(), nullptr);
        const THiliteMark ln(paramark, TUtf16String());
        g.MarkParabeg(&ln);
        TZonedString s = g.GlueToZonedString();
        s = TGluer::EmbedPara(s);
        TUtf16String res = TGluer::GlueToString(s);
        if (res.StartsWith(paramark)) {
            res = res.substr(paramark.size());
        }
        return res;
    }

    void TPreviewReplacer::TImpl::DoWork(TReplaceManager* manager) {
        Fill(manager->GetContext());
        //it's a post-replacer, so other specattrs are already set via manager->Commit, now add previewjson to them
        if (PreviewJson) {
            manager->GetExtraSnipAttrs().AppendSpecAttr("stann", "1");
            manager->GetExtraSnipAttrs().SetPreviewJson(PreviewJson);
            if (manager->GetContext().Cfg.NeedFormRawPreview()) {
                manager->GetExtraSnipAttrs().SetRawPreview(RawPreviewFiller->GetRawPreview());
            }
            manager->SetMarker(MRK_PREVIEW);
        }
    }

    TPreviewReplacer::TPreviewReplacer(const TContentPreviewViewer& sentenceViewer, const TForumMarkupViewer& forums)
      : IReplacer("preview")
      , Impl(new TImpl(sentenceViewer, forums))
    {
    }
    TPreviewReplacer::~TPreviewReplacer() {
    }
    void TPreviewReplacer::DoWork(TReplaceManager* manager) {
        Impl->DoWork(manager);
    }
} // namespace NSnippets
