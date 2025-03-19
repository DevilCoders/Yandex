#pragma once

#include <util/generic/string.h>
#include <util/generic/hash_set.h>
#include <util/generic/deque.h>
#include <util/charset/wide.h>
#include <util/memory/pool.h>

#include <library/cpp/numerator/numerate.h>
#include <library/cpp/html/dehtml/dehtml.h>
#include <library/cpp/html/pdoc/pds.h>

#include <kernel/indexer/face/inserter.h>
#include <kernel/indexann/protos/data.pb.h>
#include <kernel/itditp/proto/images4pages.pb.h>
#include <kernel/itditp/proto/site_recommendation_settings.pb.h>
#include <kernel/tarc/iface/tarcface.h>

#include <dict/recognize/docrec/recognizer.h>

#include <util/generic/maybe.h>

#include <array>

class TWordFilter;

const TUtf16String DOTS = u"...";

bool IsDescriptionGood(TUtf16String meta, TUtf16String title, const TWordFilter& stopWords);

class TMetaDescrHandler : public INumeratorHandler {
public:
    enum EMetaSource {
        SOURCE_FIRST = 0,
        SOURCE_HTML = 0,
        SOURCE_OG = 1,
        SOURCE_COUNT
    };

    static constexpr TStringBuf AttrNameImage = "og:image";
    static constexpr TStringBuf AttrNameImageWidth = "og:image:width";
    static constexpr TStringBuf AttrNameImageHeight = "og:image:height";

    static const size_t OG_SIZE_LIMIT = 4096;
    static const size_t NAMESPACE_SIZE_LIMIT = 1024;
    static const size_t IMAGE_URL_SIZE_LIMIT = 1024;

    static const size_t MIN_ANN_LEN = 3;
    static const size_t MAX_ANN_LEN = 120;

private:
    class TAnnSentenceNumerator;
    THolder<TAnnSentenceNumerator> AnnNumerator;
    THtProcessor* HtProcessor;

private:
    ECharset Encoding;
    const char* META_ATTR;
    const char* TITLE_ZONE;
    bool InTitle;
    bool InH1;
    bool InH2;
    TUtf16String Title;
    TString RawOgDescription;
    TString UltimateDescription; // og:description > twitter:description. (Meta)Description is parsed separetly.
    TUtf16String ParsedUltimateDescription;
    TString UltimateTitle; // og:title > twitter:title. Title is parsed separetly.
    TUtf16String ParsedUltimateTitle;
    std::array<TUtf16String, (size_t)SOURCE_COUNT> Descriptions;

    TMemoryPool OgKeyValueBuf;
    typedef TVector<std::pair<TStringBuf, TStringBuf> > TOgKeyValues;
    TOgKeyValues OgKeyValues;
    TBufferOutput OgNamespaces;
    TString OgAttrsAttr;
    TString OgNamespacesAttr;
    TUtf16String H1;
    TUtf16String H2;
    TString MetaRobots;
    TString MetaYandex;
    TString RawSiteRecommendationTitle;
    TString RawSiteRecommendationTitleFromRelap;
    TUtf16String SiteRecommendationTitle;
    TMaybe<NItdItp::TSiteRecommendationSettings> SiteRecommendationSettings;
    TMaybe<NItdItp::TSiteRecommendationCategories> RawSiteRecommendationCategories;
    TMaybe<NItdItp::TSiteRecommendationCategories> RawSiteRecommendationCategoriesFromSchemaOrg;
    TMaybe<NItdItp::TSiteRecommendationCategories> SiteRecommendationCategories;
    TMaybe<NImages::NIndex::TImages4PagesPB> SiteRecommendationImages;
    TString ZenSearchGroupingAttribute;
    TString ZenSearchObjectId;

    TDeque<TUtf16String> FullAnnotations;
    THashSet<TUtf16String> DocWords;

    bool IsGood(TUtf16String meta, const TWordFilter& stopWords);
    TUtf16String DecodeMeta(const IParsedDocProperties* ps, const TStringBuf& prop);
    TString DecodeAttr(const IParsedDocProperties* ps, const TStringBuf& prop);
    bool AddSafeString(const TStringBuf& str, TBufferOutput& out, size_t sizeLimit);
    void AddNamespace(const TStringBuf& ns);
    void AddMetadata(const TStringBuf& name, const TStringBuf& value, TBufferOutput& out);
    void HandleMeta(const THtmlChunk& chunk);
    void HandleHtmlOrHead(const THtmlChunk& chunk);
    void InsertOg(IDocumentDataInserter& inserter);
    bool InsertFactorsForSource(
        EMetaSource source,
        IDocumentDataInserter& inserter,
        const TWordFilter& stopWords,
        TRecognizer* recognizer,
        TRecognizer::THints* hints);
    template <class Input>
    void NumerateHtml(Input& content);
    void TryExtractAnnotations(TStringBuf name, TStringBuf content);

    void AddRecommendationsTag(const TStringBuf& tag);
    void AddRecommendationsCategory(const TStringBuf& category, TMaybe<NItdItp::TSiteRecommendationCategories>* categories);
    void SetRecommendationsKill(const TStringBuf& val);
    void SetRecommendationsImage(const TStringBuf& imageUrl, const TString& siteRecommendationsImageSource);
    void SetRecommendationsOgImage(const TString& imageUrl, const TMaybe<ui32>& imageWidth, const TMaybe<ui32>& imageHeight);
    void UpdateOgImageAttrs(
        const TStringBuf& ogAttrName,
        const TStringBuf& ogAttrValue,
        TMaybe<TString>* ogImageUrl,
        TMaybe<ui32>* ogImageWidth,
        TMaybe<ui32>* ogImageHeight);

public:
    TMetaDescrHandler(ECharset encoding = CODES_UTF8, THtProcessor* htProcessor = nullptr);
    ~TMetaDescrHandler();

    const TUtf16String& GetMeta(EMetaSource source = SOURCE_HTML) const;

    const TUtf16String& GetParsedUltimateDescription() const;

    const TUtf16String& GetParsedUltimateTitle() const;

    const char* GetAttrPrefix(EMetaSource source) const;

    const TUtf16String& GetTitle() const;

    void OnMoveInput(const THtmlChunk& chunk, const TZoneEntry* zone, const TNumerStat&) override;

    void OnTokenStart(const TWideToken& tok, const TNumerStat&) override;

    void OnSpaces(TBreakType , const wchar16* tok, unsigned length, const TNumerStat&) override;

    void OnTextEnd(const IParsedDocProperties* ps, const TNumerStat& ) override;

    void InsertFactors(
        IDocumentDataInserter& inserter,
        const TWordFilter& stopWords,
        TRecognizer* recognizer,
        TRecognizer::THints* hints);

    void InsertFactors(
        IDocumentDataInserter& inserter,
        const TWordFilter& stopWords);

    void InsertFactors(
        IDocumentDataInserter& inserter,
        const TWordFilter& stopWords,
        TRecognizer& recognizer,
        const TStringBuf& urlHint,
        const ELanguage langHint);

    bool DumpAnnData(NIndexAnn::TIndexAnnSiteData& annSiteData) const;

    const TUtf16String& GetH1() const {
        return H1;
    }

    const TUtf16String& GetH2() const {
        return H2;
    }

    const TString& GetMetaRobots() const {
        return MetaRobots;
    }

    const TString& GetMetaYandex() const {
        return MetaYandex;
    }

    const TMaybe<NItdItp::TSiteRecommendationSettings>& GetSiteRecommendationSettings() const {
        return SiteRecommendationSettings;
    }

    const TMaybe<NItdItp::TSiteRecommendationCategories>& GetSiteRecommendationCategories() const {
        return SiteRecommendationCategories;
    }

    const TMaybe<NImages::NIndex::TImages4PagesPB>& GetSiteRecommendationImages() const {
        return SiteRecommendationImages;
    }

    const TString& GetZenSearchGroupingAttribute() const noexcept {
        return ZenSearchGroupingAttribute;
    }
};
