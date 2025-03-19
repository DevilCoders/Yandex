#include "metadescr.h"

#include <kernel/itditp/utils/site_recommendation_setting_limits.h>
#include <kernel/itditp/utils/site_recommendation_setting_restrictions.h>

#include <library/cpp/html/pdoc/pds.h>
#include <library/cpp/html/entity/htmlentity.h>
#include <library/cpp/tokenizer/tokenizer.h>
#include <library/cpp/stopwords/stopwords.h>

#include <util/string/split.h>
#include <util/generic/string.h>
#include <util/generic/hash.h>
#include <util/generic/utility.h>

namespace {
    class TEQInfo : private ITokenHandler {
    public:
        const TWordFilter& StopwordsFilter;
        THashMap<TUtf16String, size_t> WordFrq;
        size_t Total = 0;
        size_t TokenCount = 0;

    private:
        static const wchar16 CapitalYo = 0x0401;
        static const wchar16 SmallYo = 0x0451;
        static const wchar16 CapitalYe = 0x0415;
        static const wchar16 SmallYe = 0x0435;

        void DeyoInplace(TUtf16String& s) {
            wchar16* p = s.begin();
            const wchar16* e = s.end();
            while (p != e) {
                if (*p == CapitalYo) {
                    *p = CapitalYe;
                } else if (*p == SmallYo) {
                    *p = SmallYe;
                }
                ++p;
            }
        }

        void OnWord(const TUtf16String& word) {
            ++TokenCount;
            if (word.size() >= 2 && !StopwordsFilter.IsStopWord(word)) {
                ++WordFrq[word];
                ++Total;
            }
        }

        void OnToken(const TWideToken& multiToken, size_t /*origleng*/, NLP_TYPE type) override {
            const size_t leng = multiToken.Leng;
            const wchar16* const token = multiToken.Token;
            if (!leng)
                return;
            if (type == NLP_WORD || type == NLP_MARK || type == NLP_INTEGER || type == NLP_FLOAT || type == NLP_MARK) {
                if (multiToken.SubTokens.size() <= 1)
                    OnWord(TUtf16String(token, leng));
                else {
                    for (size_t i = 0; i < multiToken.SubTokens.size(); ++i) {
                        const TCharSpan& s = multiToken.SubTokens[i];
                        OnWord(TUtf16String(token + s.Pos, s.Len + s.SuffixLen));
                    }
                }
            }
        }

    public:
        TEQInfo(const TUtf16String& text, const TWordFilter& stopwordsFilter)
            : StopwordsFilter(stopwordsFilter)
        {
            TUtf16String t = text;
            DeyoInplace(t);
            t.to_lower();
            TNlpTokenizer tokenizer(*this, false);
            tokenizer.Tokenize(t);
        }

        size_t CountEqWords(const TEQInfo& other) const {
            if (WordFrq.size() < other.WordFrq.size()) {
                return other.CountEqWords(*this);
            }
            size_t eqCount = 0;
            for (const auto& wordFrq : other.WordFrq) {
                auto it = WordFrq.find(wordFrq.first);
                if (it != WordFrq.end()) {
                    eqCount += Min(wordFrq.second, it->second);
                }
            }
            return eqCount;
        }

        double MaxWordFrq() const {
            if (Total == 0) {
                return 0.0;
            }
            size_t maxCount = 0;
            for (const auto& wordFrq : WordFrq) {
                maxCount = Max(wordFrq.second, maxCount);
            }
            return double(maxCount) / double(Total);
        }
    };
}


static const size_t MAX_RECOGNIZED_LANGS = 5;

static const TUtf16String WIDE_SPACE = u" ";

static void WtrokaFix(TUtf16String& wtr) {
    if (!wtr.empty() && wtr.back() == '\0')
        wtr.pop_back();
}

static bool IsClearEqual(const TUtf16String& title, const TUtf16String& meta) {
    TUtf16String::const_iterator tit = title.begin();
    TUtf16String::const_iterator mit = meta.begin();

    while (true) {
        while (!IsAlnum(*tit) && tit != title.end())
            ++tit;
        while (!IsAlnum(*mit) && mit != meta.end())
            ++mit;

        if (tit != title.end() && mit != meta.end()) {
            if (ToLower(*tit) == ToLower(*mit)) {
                ++tit;
                ++mit;
            } else
                return false;
        } else if (tit == title.end() && mit == meta.end())
            return true;
        else
            return false;
    }
}

static int CountPuncts(const TUtf16String& meta) {
    int result = 0;
    for (TUtf16String::const_iterator it = meta.begin(); it != meta.end(); ++it)
        if (IsPunct(*it))
            ++result;
    return result;
}

bool IsDescriptionGood(TUtf16String meta, TUtf16String title, const TWordFilter& stopWords) {
    WtrokaFix(title);
    Strip(title);
    WtrokaFix(meta);
    Strip(meta);

    if (meta.size() <= 15 || title == meta || IsClearEqual(title, meta))
        return false;

    TEQInfo mEQ(meta, stopWords);
    if (mEQ.TokenCount <= 4)
        return false;

    const int punctCount = CountPuncts(meta);
    if (double(punctCount) / double(mEQ.TokenCount) >= 0.66)
        return false;

    TEQInfo tEQ(title, stopWords);
    const double lenCmp = double(mEQ.TokenCount) / double(tEQ.TokenCount);
    if (mEQ.TokenCount <= 6 && lenCmp <= 0.6)
        return false;

    if (mEQ.MaxWordFrq() >= 0.33)
        return false;

    if (lenCmp < 1.6 && tEQ.Total > 0 && mEQ.CountEqWords(tEQ) > tEQ.Total * 0.6)
        return false;

    size_t totalWords = mEQ.Total;
    size_t diffWords = mEQ.WordFrq.size();
    if (totalWords != 0) {
        if (static_cast<double>(diffWords) / totalWords <= 0.5)
            return false;
    }

    return true;
}

static TTempBuf DecodeRawAttr(const IParsedDocProperties* ps, const TStringBuf& prop)
{
    TCharTemp tempBuf(prop.size());
    unsigned lenbuf = HtEntDecodeToChar(ps->GetCharset(), prop.data(), prop.size(), tempBuf.Data());
    TString s = WideToUTF8(tempBuf.Data(), (size_t)lenbuf);
    size_t toLen = prop.size() * 5;
    TTempBuf to(toLen);
    THtmlStripper::Strip(HSM_ENTITY, to.Data(), toLen, s.data(), s.size(), CODES_UTF8);
    to.SetPos(toLen);
    return to;
}

class TMetaDescrHandler::TAnnSentenceNumerator : public INumeratorHandler {
public:
    TAnnSentenceNumerator(TDeque<TUtf16String>& sentences)
        : Sentences(sentences)
        , CurrentSentence()
    {
    }

    void OnTokenStart(const TWideToken& tok, const TNumerStat& /*stat*/) override {
        if (!CurrentSentence.empty()) {
            CurrentSentence.append(WIDE_SPACE);
        }

        CurrentSentence.append(TWtringBuf(tok.Token, tok.Leng));
    }

    void OnSpaces(TBreakType type, const wchar16* /*val*/, unsigned /*len*/, const TNumerStat& /*stat*/) override {
        if ((IsSentBrk(type) || IsParaBrk(type))) {
            FlushSentence();
        }
    }

    void FlushSentence() {
        if (CurrentSentence.size() >= TMetaDescrHandler::MIN_ANN_LEN) {
            CurrentSentence.to_lower();
            Sentences.push_back(CurrentSentence);
        }
        CurrentSentence.clear();
    }

private:
    TDeque<TUtf16String>& Sentences;
    TUtf16String CurrentSentence;
};

TMetaDescrHandler::TMetaDescrHandler(ECharset encoding, THtProcessor* htProcessor)
    : HtProcessor(htProcessor)
    , Encoding(encoding)
    , META_ATTR(ToString(AZ_ABSTRACT))
    , TITLE_ZONE(ToString(AZ_TITLE))
    , InTitle(false)
    , InH1(false)
    , InH2(false)
    , Title()
    , OgKeyValueBuf(OG_SIZE_LIMIT)
{
    OgKeyValues.reserve(20);
    AnnNumerator.Reset(new TAnnSentenceNumerator(FullAnnotations));
}

TMetaDescrHandler::~TMetaDescrHandler() = default;

TUtf16String TMetaDescrHandler::DecodeMeta(const IParsedDocProperties* ps, const TStringBuf& prop)
{
    TUtf16String result;
    if (!prop)
        return result;
    TTempBuf tmp = DecodeRawAttr(ps, prop);
    result = UTF8ToWide(TStringBuf(tmp.Data(), tmp.Filled()));
    if (result.size() > 2000)
        result.remove(2000);
    return result;
}

TString TMetaDescrHandler::DecodeAttr(const IParsedDocProperties* ps, const TStringBuf& prop)
{
    TString result;
    if (!prop)
        return result;
    TTempBuf tmp = DecodeRawAttr(ps, prop);
    result = TString(tmp.Data(), tmp.Filled());
    return result;
}

bool TMetaDescrHandler::AddSafeString(const TStringBuf& str, TBufferOutput& out, size_t sizeLimit)
{
    TBuffer& buf = out.Buffer();
    size_t lastSize = buf.size();
    if (str.size() + lastSize + 1 > sizeLimit) {
        return false;
    }
    if (lastSize) {
        out << "\t";
        lastSize += 1;
    }
    out << str;
    char* ptr = buf.data() + lastSize;
    char* end = buf.data() + (buf.size());
    for (; ptr < end; ++ptr) {
        if ((unsigned char)*ptr < (unsigned char)0x20) {
            *ptr = ' ';
        }
    }

    return true;
}

void TMetaDescrHandler::AddNamespace(const TStringBuf& ns)
{
    AddSafeString(ns, OgNamespaces, NAMESPACE_SIZE_LIMIT);
}

template <class Input>
void TMetaDescrHandler::NumerateHtml(Input& content) {
    if (HtProcessor != nullptr) {
        NumerateHtmlSimple(*AnnNumerator, *HtProcessor, content.data(), content.size());
    } else {
        NumerateHtmlSimple(*AnnNumerator, content.data(), content.size());
    }
}

void TMetaDescrHandler::TryExtractAnnotations(TStringBuf name, TStringBuf content) {
    if (name != "description" && name != "keywords") {
        return;
    }
    if (Encoding == CODES_UNKNOWN) {
        return;
    }

    TString recoded;
    if (Encoding != CODES_UTF8) {
        TTempBuf tmp(content.size() * 5);
        const size_t len = HtDecodeAttrToUtf8(Encoding, content.data(), content.size(), tmp.Data(), tmp.Size());
        recoded = TString(tmp.Data(), len);
        content = recoded;
    }

    if (name == "description") {
        NumerateHtml(content);
        AnnNumerator->FlushSentence();
    } else if (name == "keywords") {
        TStringBuf kwBuf(content.data(), content.size());
        TStringBuf curKW = kwBuf.NextTok(',');
        while (curKW) {
            NumerateHtml(curKW);
            AnnNumerator->FlushSentence();
            curKW = kwBuf.NextTok(',');
        }
    }
}

void TMetaDescrHandler::HandleMeta(const THtmlChunk& chunk) {
    TStringBuf prop;
    TStringBuf itemProp;
    TStringBuf name;
    TStringBuf val;
    for (size_t i = 0; i < chunk.AttrCount; ++i) {
        const NHtml::TAttribute& attr = chunk.Attrs[i];
        TStringBuf attrName(chunk.text + attr.Name.Start, chunk.text + attr.Name.Start + attr.Name.Leng);
        TStringBuf attrValue(chunk.text + attr.Value.Start, chunk.text + attr.Value.Start + attr.Value.Leng);
        if (attrName == "property") {
            prop = attrValue;
        } else if (attrName == "itemprop") {
            itemProp = attrValue;
        } else if (attrName == "name") {
            name = attrValue;
        } else if (attrName == "content") {
            val = attrValue;
        }
    }

    if (name == "robots") {
        MetaRobots = TString{val};
    } else if (name == "yandex") {
        MetaYandex = TString{val};
    } else if (!name.empty() && !val.empty()) {
        TryExtractAnnotations(name, val);
    }

    if (val.empty()) {
        return;
    }

    if (prop == "yandex_recommendations_title") {
        RawSiteRecommendationTitle = val;
    } else if (prop == "yandex_recommendations_tag") {
        AddRecommendationsTag(val);
    } else if (prop == "yandex_recommendations_category") {
        AddRecommendationsCategory(val, &RawSiteRecommendationCategories);
    } else if (itemProp == "about" || itemProp == "articleSection") {
        AddRecommendationsCategory(val, &RawSiteRecommendationCategoriesFromSchemaOrg);
    } else if (prop == "yandex_recommendations_kill") {
        SetRecommendationsKill(val);
    } else if (prop == "yandex_recommendations_image"){
        SetRecommendationsImage(val, "site_recommendations_image");
    } else if (name == "relap-image") {
        SetRecommendationsImage(val, "relap_image");
    } else if (name == "relap-title") {
        RawSiteRecommendationTitleFromRelap = val;
    } else if (prop == "og:description") {
        RawOgDescription = val;
        UltimateDescription = val;
    } else if (name == "twitter:description") {
        if (UltimateDescription.empty()) {
            UltimateDescription = val;
        }
    } else if (prop == "twitter:title") {
        if (UltimateTitle.empty()) {
            UltimateTitle = val;
        }
    } else if (prop.StartsWith("og:") || prop.StartsWith("article:") || prop.StartsWith("video:duration")) {
        if (prop == "og:title") {
            UltimateTitle = val;
        }
        prop = OgKeyValueBuf.AppendString(prop);
        val = OgKeyValueBuf.AppendString(val);
        OgKeyValues.push_back(std::make_pair(prop, val));
    } else if (prop == "zen_grouping_attr") {
        ZenSearchGroupingAttribute = val;
    } else if (prop == "zen_object_id") {
        ZenSearchObjectId = val;
    }
}

void TMetaDescrHandler::AddRecommendationsTag(const TStringBuf& tag) {
    if (!NItdItp::IsCorrectTagString(tag)) {
        return;
    }

    if (SiteRecommendationSettings.Empty()) {
        SiteRecommendationSettings.ConstructInPlace();
    } else if (SiteRecommendationSettings->TagsSize() >= NItdItp::TSiteRecommendationSettingLimits::MaxTags) {
        return;
    }

    SiteRecommendationSettings->AddTags(TString{tag});
}

void TMetaDescrHandler::AddRecommendationsCategory(const TStringBuf& category, TMaybe<NItdItp::TSiteRecommendationCategories>* categories) {
    if (categories->Empty())  {
        categories->ConstructInPlace();
    } else if (categories->GetRef().CategoriesSize() >= NItdItp::TSiteRecommendationSettingLimits::MaxCategories) {
        return;
    }

    categories->GetRef().AddCategories(TString{category});
}

void TMetaDescrHandler::SetRecommendationsKill(const TStringBuf& val) {
    if (val != "1") {
        return;
    }

    if (SiteRecommendationSettings.Empty()) {
        SiteRecommendationSettings.ConstructInPlace();
    }

    SiteRecommendationSettings->SetKill(true);
}

void TMetaDescrHandler::SetRecommendationsImage(const TStringBuf& imageUrl, const TString& siteRecommendationsImageSource) {
    if (imageUrl.size() > IMAGE_URL_SIZE_LIMIT) {
        return;
    }

    if (SiteRecommendationImages.Empty()) {
        SiteRecommendationImages.ConstructInPlace();
    }

    if (FindIfPtr(
        SiteRecommendationImages->GetImages().GetImageDescriptors(),
        [&siteRecommendationsImageSource](const NImages::NIndex::TImageDescriptorPB& descr) { return descr.GetImageSource() == siteRecommendationsImageSource; }))
    {
        return;
    }

    NImages::NIndex::TImageDescriptorPB* descr = SiteRecommendationImages->MutableImages()->AddImageDescriptors();
    descr->SetImageUrl(TString{imageUrl});
    descr->SetImageSource(siteRecommendationsImageSource);
}

void TMetaDescrHandler::UpdateOgImageAttrs(
    const TStringBuf& ogAttrName,
    const TStringBuf& ogAttrValue,
    TMaybe<TString>* ogImageUrl,
    TMaybe<ui32>* ogImageWidth,
    TMaybe<ui32>* ogImageHeight)
{
    if (ogAttrName == AttrNameImage) {
        if (ogAttrValue.size() <= IMAGE_URL_SIZE_LIMIT && ogImageUrl->Empty()) {
            *ogImageUrl = TString{ogAttrValue};
        }
    } else {
        Y_ASSERT(EqualToOneOf(ogAttrName, AttrNameImageWidth, AttrNameImageHeight));
        TMaybe<ui32>* imageValue = (ogAttrName == AttrNameImageWidth ? ogImageWidth : ogImageHeight);
        if (imageValue->Defined()) {
            return;
        }
        ui32 value = 0;
        if (TryFromString<ui32>(ogAttrValue, value) && value > 0) {
            *imageValue = value;
        }
    }
}

void TMetaDescrHandler::SetRecommendationsOgImage(const TString& imageUrl, const TMaybe<ui32>& imageWidth, const TMaybe<ui32>& imageHeight) {
    if (SiteRecommendationImages.Empty()) {
        SiteRecommendationImages.ConstructInPlace();
    }

    NImages::NIndex::TImageDescriptorPB* descr = SiteRecommendationImages->MutableImages()->AddImageDescriptors();
    descr->SetImageUrl(imageUrl);
    descr->SetImageSource("og_image");
    if (imageWidth.Defined() && imageHeight.Defined()) {
        descr->SetWidth(*imageWidth);
        descr->SetHeight(*imageHeight);
    }
}

void TMetaDescrHandler::HandleHtmlOrHead(const THtmlChunk& chunk) {
    for (size_t i = 0; i < chunk.AttrCount; ++i) {
        const NHtml::TAttribute& attr = chunk.Attrs[i];
        TStringBuf attrName(chunk.text + attr.Name.Start, chunk.text + attr.Name.Start + attr.Name.Leng);
        TStringBuf attrValue(chunk.text + attr.Value.Start, chunk.text + attr.Value.Start + attr.Value.Leng);
        if (!attrValue) {
            continue;
        }
        if (attrName == "prefix") {
            AddNamespace(attrValue);
            continue;
        }
        if (attrName.StartsWith("xmlns:")) {
            attrName = attrName.substr(6);
            if (!!attrName) {
                TTempBuf buf((attrName.size()) + (attrValue.size()) + 2);
                buf.Append(attrName.data(), attrName.size());
                buf.Append(": ", 2);
                buf.Append(attrValue.data(), attrValue.size());
                AddNamespace(TStringBuf(buf.Data(), buf.Filled()));
            }
        }
    }
}

bool TMetaDescrHandler::InsertFactorsForSource(
        EMetaSource source,
        IDocumentDataInserter& inserter,
        const TWordFilter& stopWords,
        TRecognizer* recognizer,
        TRecognizer::THints* hints)
{
    const TUtf16String& description = GetMeta(source);
    if (!description) {
        return false;
    }

    TString strPrefix(GetAttrPrefix(source));
    inserter.StoreTextArchiveDocAttr((strPrefix + "_descr").data(), WideToUTF8(description));
    const bool isGood = IsDescriptionGood(description, Title, stopWords);
    inserter.StoreTextArchiveDocAttr((strPrefix + "_quality").data(), isGood ? "yes" : "no");

    if (!recognizer || !hints || description.size() < 10) {
        return isGood;
    }

    TStringStream langsEncoded;
    TRecognizer::TLanguages langs;
    bool hasUsefulLangs = false;
    recognizer->RecognizeLanguage(description.begin(), description.end(), langs, *hints);
    if (langs.size() > MAX_RECOGNIZED_LANGS) {
        langs.resize(MAX_RECOGNIZED_LANGS);
    }
    for (TRecognizer::TLanguages::const_iterator ii = langs.begin(), end = langs.end(); ii != end; ++ii) {
        langsEncoded << NameByLanguage(ii->Language) << "\t" << ii->Coverage << "\t";
        if (ii->Language != LANG_UNK) {
            hasUsefulLangs = true;
        }
    }
    if (hasUsefulLangs) {
        inserter.StoreTextArchiveDocAttr(strPrefix + "_langs", langsEncoded.Str());
    }

    return true;
}

const TUtf16String& TMetaDescrHandler::GetMeta(EMetaSource source) const {
    Y_ASSERT(source >= SOURCE_FIRST && source < SOURCE_COUNT);
    return Descriptions[(size_t)source];
}

const TUtf16String& TMetaDescrHandler::GetParsedUltimateDescription() const {
    return GetMeta().empty() ? ParsedUltimateDescription : GetMeta();
}

const TUtf16String& TMetaDescrHandler::GetParsedUltimateTitle() const {
    return Title.empty() ? ParsedUltimateTitle : Title;
}

const char* TMetaDescrHandler::GetAttrPrefix(EMetaSource source) const {
    return source == SOURCE_OG ? "og" : "meta";
}

const TUtf16String& TMetaDescrHandler::GetTitle() const {
    return Title;
}

void TMetaDescrHandler::OnMoveInput(const THtmlChunk& chunk, const TZoneEntry* zone, const TNumerStat&) {
    if (chunk.Tag && chunk.AttrCount) {
        if (chunk.Tag->id() == HT_META) {
            HandleMeta(chunk);
        } else if (chunk.Tag->id() == HT_HEAD || chunk.Tag->id() == HT_HTML) {
            HandleHtmlOrHead(chunk);
        }
    }

    if (zone && !zone->OnlyAttrs && zone->Name && strcmp(TITLE_ZONE, zone->Name) == 0)
        InTitle = zone->IsOpen;
    if (chunk.Tag && (chunk.Tag->id() == HT_H1)) {
        InH1 = (chunk.leng >= 2) && (chunk.text[1] != '/');
    }
    if (chunk.Tag && (chunk.Tag->id() == HT_H2)) {
        InH2 = (chunk.leng >= 2) && (chunk.text[1] != '/');
    }
}

void TMetaDescrHandler::OnTokenStart(const TWideToken& tok, const TNumerStat&) {
    if (InTitle) {
        Title.append(tok.Token, tok.Leng);
    }
    if (InH1) {
        H1.append(tok.Token, tok.Leng);
    }
    if (InH2) {
        H2.append(tok.Token, tok.Leng);
    }

    TUtf16String tokenStr(tok.Token, tok.Leng);
    tokenStr.to_lower();
    TWtringBuf tokenForSplit(tokenStr.data(), tokenStr.size());
    TWtringBuf word = tokenForSplit.NextTok(' ');
    while (word) {
        DocWords.insert(ToWtring(word));
        word = tokenForSplit.NextTok(' ');
    }

}

void TMetaDescrHandler::OnSpaces(TBreakType , const wchar16* tok, unsigned length, const TNumerStat&) {
    if (InTitle)
        Title.append(tok, length);
    if (InH1)
        H1.append(tok, length);
    if (InH2)
        H2.append(tok, length);
}

void TMetaDescrHandler::OnTextEnd(const IParsedDocProperties* ps, const TNumerStat& )
{
    auto metaValue = ps->GetValueAtIndex(META_ATTR, 0);
    if (!metaValue.empty()) {
        Descriptions[SOURCE_HTML] = DecodeMeta(ps, metaValue);
    }

    if (!!RawOgDescription) {
        Descriptions[SOURCE_OG] = DecodeMeta(ps, RawOgDescription);
    }

    ParsedUltimateDescription = DecodeMeta(ps, UltimateDescription);
    ParsedUltimateTitle = DecodeMeta(ps, UltimateTitle);

    if (!RawSiteRecommendationTitle.empty()) {
        SiteRecommendationTitle = DecodeMeta(ps, RawSiteRecommendationTitle);
    } else if (!RawSiteRecommendationTitleFromRelap.empty()) {
        SiteRecommendationTitle = DecodeMeta(ps, RawSiteRecommendationTitleFromRelap);
    }

    if (RawSiteRecommendationCategories.Defined() || RawSiteRecommendationCategoriesFromSchemaOrg.Defined()) {
        const auto& categories = (RawSiteRecommendationCategories.Defined()
            ? RawSiteRecommendationCategories->GetCategories()
            : RawSiteRecommendationCategoriesFromSchemaOrg->GetCategories());

        for (const auto& rawCategory : categories) {
            TString category = WideToUTF8(DecodeMeta(ps, rawCategory));
            if (!NItdItp::IsCorrectCategoryString(category)) {
                continue;
            }
            if (SiteRecommendationCategories.Empty()) {
                SiteRecommendationCategories.ConstructInPlace();
            }
            SiteRecommendationCategories->AddCategories(category);
        }
    }

    TBuffer& nsBuf = OgNamespaces.Buffer();
    if (!nsBuf.Empty()) {
        OgNamespacesAttr = DecodeAttr(ps, TStringBuf(nsBuf.Data(), nsBuf.Size()));
    }

    static constexpr TStringBuf AttrNameUrl = "og:url";

    TMaybe<TString> ogImageUrl;
    TMaybe<ui32> ogImageWidth;
    TMaybe<ui32> ogImageHeight;

    TBufferOutput ogAttrs(OG_SIZE_LIMIT);
    bool dontAcceptSubprops = true;
    size_t subpropsStartOffset = 0;
    for (TOgKeyValues::iterator ii = OgKeyValues.begin(), iiend = OgKeyValues.end(); ii != iiend; ++ii) {
        int level = 0;
        TTempBuf nameBuf = DecodeRawAttr(ps, ii->first);
        TStringBuf name(nameBuf.Data(), nameBuf.Filled());
        TTempBuf valueBuf = DecodeRawAttr(ps, ii->second);
        TStringBuf value(valueBuf.Data(), valueBuf.Filled());

        if (EqualToOneOf(name, AttrNameImage, AttrNameImageWidth, AttrNameImageHeight)) {
            UpdateOgImageAttrs(name, value, &ogImageUrl, &ogImageWidth, &ogImageHeight);
        }

        const char* ptr = name.data();
        const char* end = name.data() + (name.size());
        for (; ptr < end; ++ptr) {
            if (*ptr == ':')
                ++level;
        }
        bool isSubProp = (level > 1);

        if (isSubProp && dontAcceptSubprops)
            continue;

        if (!isSubProp && EqualToOneOf(name, AttrNameImage, AttrNameUrl)) {
            dontAcceptSubprops = true;
            continue;
        }

        if (!AddSafeString(name, ogAttrs, OG_SIZE_LIMIT) || !AddSafeString(value, ogAttrs, OG_SIZE_LIMIT)) {
            dontAcceptSubprops = true;
            ogAttrs.Buffer().Resize(subpropsStartOffset);
        } else if (!isSubProp) {
            dontAcceptSubprops = false;
            subpropsStartOffset = ogAttrs.Buffer().Size();
        }
    }
    OgAttrsAttr = TString(ogAttrs.Buffer().Data(), ogAttrs.Buffer().Size());

    if (ogImageUrl.Defined()) {
        SetRecommendationsOgImage(*ogImageUrl, ogImageWidth, ogImageHeight);
    }
}

void TMetaDescrHandler::InsertOg(IDocumentDataInserter& inserter)
{
    // OgNamespaces ("og_namespaces") has been removed, as no one has figured out how to use it yet
    if (!!OgAttrsAttr) {
        inserter.StoreTextArchiveDocAttr("og_attrs", OgAttrsAttr);
    }
}

void TMetaDescrHandler::InsertFactors(
    IDocumentDataInserter& inserter,
    const TWordFilter& stopWords,
    TRecognizer* recognizer,
    TRecognizer::THints* hints)
{
    InsertFactorsForSource(SOURCE_HTML, inserter, stopWords, recognizer, hints);
    if (Descriptions[SOURCE_HTML] != Descriptions[SOURCE_OG]) {
        InsertFactorsForSource(SOURCE_OG, inserter, stopWords, recognizer, hints);
    }
    else if (!!Descriptions[SOURCE_HTML]) {
        inserter.StoreTextArchiveDocAttr("og_see_meta_descr", "yes");
    }
    if (!SiteRecommendationTitle.empty()) {
        inserter.StoreTextArchiveDocAttr("site_recommendation_title", WideToUTF8(SiteRecommendationTitle));
    }
    if (!ZenSearchObjectId.empty()) {
        inserter.StoreTextArchiveDocAttr("zen_object_id", ZenSearchObjectId);
    }
    InsertOg(inserter);
}

void TMetaDescrHandler::InsertFactors(
    IDocumentDataInserter& inserter,
    const TWordFilter& stopWords)
{
    InsertFactors(inserter, stopWords, nullptr, nullptr);
}

void TMetaDescrHandler::InsertFactors(
    IDocumentDataInserter& inserter,
    const TWordFilter& stopWords,
    TRecognizer& recognizer,
    const TStringBuf& urlHint,
    const ELanguage langHint)
{
    TRecognizer::THints hints;
    if (langHint != LANG_UNK) {
        hints.HtmlLanguage = NameByLanguage(langHint);
    }
    TString szUrl = TString{urlHint};
    hints.Url = szUrl.data();
    InsertFactors(inserter, stopWords, &recognizer, &hints);
}

bool TMetaDescrHandler::DumpAnnData(NIndexAnn::TIndexAnnSiteData& annSiteData) const {
    if (FullAnnotations.empty()) {
        return false;
    }

    NIndexAnn::TRegionData recData;
    recData.SetRegion(0);
    auto streamData = recData.MutableMetaTagSentence();
    Y_UNUSED(streamData);

    NIndexAnn::TAnnotationRec record;
    record.AddData()->CopyFrom(recData);

    bool smthFound = false;
    TVector<TUtf16String> sentWords;
    for (const auto& text : FullAnnotations) {
        sentWords.clear();
        StringSplitter(text).SplitByString(WIDE_SPACE.data()).AddTo(&sentWords);

        TUtf16String curSent;
        for (const auto& word : sentWords) {
            if (DocWords.contains(word)) {
                continue;
            }
            smthFound = true;
            if (curSent.empty()) {
                curSent = word;
            } else if (curSent.size() + 1 + word.size() > MAX_ANN_LEN) {
                record.SetText(WideToUTF8(curSent));
                annSiteData.AddRecs()->CopyFrom(record);
                curSent = word;
            } else {
                curSent += WIDE_SPACE;
                curSent += word;
            }
        }
        if (curSent.size() >= MIN_ANN_LEN) {
            record.SetText(WideToUTF8(curSent));
            annSiteData.AddRecs()->CopyFrom(record);
        }
    }
    return smthFound;
}
