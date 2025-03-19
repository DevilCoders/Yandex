#pragma once

#include "enums.h"

#include <library/cpp/langmask/langmask.h>

#include <kernel/snippets/iface/configparams.h>
#include <kernel/snippets/idl/enums.h>
#include <kernel/snippets/iface/sentsfilter.h>
#include <kernel/snippets/iface/geobase.h>

#include <kernel/search_daemon_iface/facetype.h>

#include <kernel/factors_util/factors_util.h>
#include <kernel/factor_storage/factor_storage.h>
#include <kernel/matrixnet/mn_sse.h>

#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/hash.h>
#include <library/cpp/langs/langs.h>

class TWordFilter;

namespace NNeuralNetApplier
{
    class TModel;
}

namespace NSc
{
    class TValue;
}

namespace NSnippets
{

    class TMxNetFormula;
    class TLinearFormula;
    class TFormula;

    enum EInfoRequestTableType
    {
        INFO_XML,
        INFO_JSON,
        INFO_TABSEP
    };

    const int MAX_SNIP_COUNT = 4;

    struct TSnipInfoReqParams
    {
        EInfoRequestType RequestType = INFO_NONE;
        ui32 PageNumber = 1;
        bool UnpackedOnly = false;  // INFO_SNIPPET_HITS
        bool ShowMarkup = false;    // INFO_SNIPPET_HITS
        bool IsLinkArchive = false; // INFO_SNIPPET_HITS
        TString AlgoName;    // INFO_SNIPPETS
        EInfoRequestTableType TableType = INFO_XML;
        bool OnlySkipRestr = false;
        bool Unpaged = false;
    };

    using TRetexts = THashMap<TString, std::pair<TUtf16String, TUtf16String>>;

    enum EDelimiterType
    {
        DELIM_PIPE,
        DELIM_COLONS,
        DELIM_DASH,
        DELIM_HYPHEN,
        DELIM_COUNT
    };

    struct TRankingFactors {
    private:
        TVector<float> FactorsRaw;
        TFactorDomain FactorDomain;
        TSimpleSharedPtr<TFactorView> FactorsView;

    public:
        void Load(const NProto::TSnipReqParams& in);
        const float* GetRawFactors() const;
        float operator[](size_t i) const;
        size_t Size() const;
    };

    struct TFactorsToDump {
        bool DumpFormulaWeight = true;
        TVector<std::pair<size_t, size_t>> FactorRanges;
    };

    class TConfig: private TNonCopyable
    {
    private:
        struct TImpl;

        THolder<TImpl> Impl;

    public:
        explicit TConfig(const TConfigParams& params = TConfigParams());
        ~TConfig();

        //img stuff
        bool ImgSearch() const;
        bool IsPolitician() const;
        TLangMask GetQueryLangMask() const;
        ui64 GetRelevRegion() const;

        //video stuff
        bool VideoSearch() const;
        bool VideoTitles() const;
        bool VideoSnipAlgo() const;
        bool IsVideoExp() const;
        bool VideoLength() const;
        bool RequireSentAttrs() const;
        bool VideoHide() const;

        void LogPerformance(const TString& event) const;

        ui32 GetDocId() const;
        TString GetSnipParamsForDebug() const;
        const TWordFilter& GetStopWordsFilter() const;
        const NSc::TValue& GetEntityData() const;
        const ISentsFilter* GetSentsFilter() const;
        const IGeobase* GetGeobase() const;

        inline EInfoRequestType GetInfoRequestType() const {
            return GetInfoRequestParams().RequestType;
        }
        bool DebugOnlySkipRestr() const {
            return GetInfoRequestParams().OnlySkipRestr;
        }

        //foobar stuff
        bool Foo() const;
        bool Bar() const;
        bool Baz() const;
        bool Qux() const;
        bool Fred() const;

        //length stuff
        int GetMaxSnipCount() const;
        int GetMaxSnipRowCount() const;
        int GetMaxByLinkSnipCount() const;
        ETextCutMethod GetTitleCutMethod() const;
        ETextCutMethod GetSnipCutMethod() const;
        float GetRequestedMaxSnipLen() const;
        int GetMaxTitleLen() const;
        int GetMaxDefinitionLen() const;
        int GetTitlePixelsInLine() const;
        float GetMaxTitleLengthInRows() const;
        float GetTitleFontSize() const;
        float GetSnipFontSize() const;
        ui32 GetMaxUrlLength() const;
        ui32 GetUrlCutThreshold() const;
        ui32 GetMobileHilitedUrlReduction() const;
        ui32 GetMaxUrlmenuLength() const;
        ui64 GetUserReqHash() const;
        int GetSnipWidth() const;

        const TMxNetFormula& GetAll() const;
        const TLinearFormula& GetManual() const;
        const TMxNetFormula& GetFactSnippetFormula() const;
        const TLinearFormula& GetFactSnippetManual() const;
        TFormula GetTotalFormula() const;
        TFormula GetFactSnippetTotalFormula() const;

        //random stuff
        bool SkipWordstatDebug() const;
        const TString& GetRedump() const;
        const TRetexts& GetRetexts() const;
        //Issue: SNIPPETS-2683
        bool ErasePoorAlts() const;
        bool UnpAll() const;
        bool RawAll() const;
        bool HasEntityClassRequest() const;
        bool NeedFormRawPreview() const;
        bool VideoAttrWeight() const;
        //Issue: SNIPPETS-1179
        bool TitWeight() const;
        bool IsMobileSearch() const;
        bool GetRawPassages() const;
        bool IsYandexCom() const;
        bool VideoLangHackTr() const;
        EFaceType GetFaceType() const;
        const TSnipInfoReqParams& GetInfoRequestParams() const;
        bool IsNeedExt() const;
        //Issue: SNIPPETS-1021
        bool NeedExtDiff() const;
        bool SkipExtending() const;
        //Issue: SNIPPETS-1022
        bool NeedExtStat() const;
        bool NoPreview() const;
        bool ForceStann() const;
        bool MaskBaseImagesPreview() const;
        bool DropBaseImagesPreview() const;
        bool SchemaPreview() const;
        bool SchemaPreviewSegmentsOff() const;
        bool PreviewBaseImagesHack() const;
        bool PreviewBaseImagesHackV2() const;
        bool DisallowVthumbPreview() const;
        int FStannLen() const;
        int FStannTitleLen() const;
        bool IsAllPassageHits() const;
        bool IsPaintedRawPassages() const;
        bool IsMarkParagraphs() const;
        bool IsSlovariSearch() const;
        bool DmozSnippetReplacerAllowed() const;
        bool YacaSnippetReplacerAllowed() const;
        bool IsMainPage() const;
        //Issue: SNIPPETS-1350
        bool IsUseStaticDataOnly() const;
        bool IsMainContentStaticDataSource() const;
        bool PaintNoAttrs() const;
        //Issue: SNIPPETS-1023
        //! Allow snippets candidates boosting with AZ_TELEPHONE zone
        bool BoostTelephoneCandidates() const;
        bool PaintAllPhones() const;
        //Issue: SNIPPETS-1324
        bool IsUrlOrEmailQuery() const;
        //Issue: SNIPPETS-1303
        bool IsNeedGenerateAltHeaderOnly() const;
        bool IsNeedGenerateTitleOnly() const;
        ELanguage GetForeignNavHackLang() const;
        // SNIPPETS-1078
        bool WeightedReplacers() const;
        const TString& GetTLD() const;
        ELanguage GetUILang() const;
        // Issue: SNIPPETS-1431
        bool LinksInSnip() const;
        bool VideoWatch() const;
        // SNIPPETS-1555
        bool SuppressCyrForTr() const;
        // SNIPPETS-2289
        bool PessimizeCyrForTr() const;
        //SNIPPETS-2774
        bool UnpaintTitle() const;
        // Issue: SNIPPETS-1011
        bool UseDiversity2() const;
        size_t Algo3TopLen() const;
        // Issue: SNIPPETS-1541
        bool VideoBlock() const;
        bool ForceVideoTitle() const;
        bool UseForumReplacer() const;
        bool CatalogOptOutAllowed() const;
        // SNIPPETS-1659
        bool BypassReplacers() const;
        // SNIPPETS-1657
        bool EnableRemoveDuplicates() const;
        // SNIPPETS-1531
        bool UseListSnip() const;
        bool DropListStat() const;
        bool DropListHeader() const;

        // SNIPPETS-1452
        bool UseTableSnip() const;
        bool DropTallTables() const;
        bool DropBrokenTables() const;
        bool DropTableStat() const;
        bool DropTableHeader() const;
        bool ShrinkTableRows() const;

        // BLNDR-5358
        TString GreenUrlDomainTrieKey() const;

        // SNIPPETS-1654
        bool ShareMessedSnip() const;
        bool WantToMess(size_t value) const;
        bool ShuffleHilite() const;
        bool NoHilite() const;
        bool AddHilite() const;

        // SNIPPETS-2476
        bool UseBadSegments() const;

        bool IgnoreRegionPhrase() const;

        //dump stuff
        bool IsDumpCandidates() const;
        bool IsDumpForPoolGeneration() const;
        double BagOfWordsIntersectionThreshold() const;
        bool IsDumpAllAlgo3Pairs() const;
        bool IsDumpWithUnusedFactors() const;
        bool IsDumpWithManualFactors() const;
        int DumpCoding() const;
        TFactorsToDump FactorsToDump() const; // SNIPPETS-3559
        bool IsDumpFinalFactors() const;
        bool IsDumpFinalFactorsBinary() const;

        //Issue: SNIPPETS-869
        inline bool NavYComLangHack() const {
            return true;
        }

        bool UnpackFirst10() const;
        int GetStaticAnnotationMode() const;

        //Issue: SAAS-2143
        bool ForcePureTitle() const;

        //Issue: SNIPPETS-1039
        bool NeedDecapital() const {
            return true;
        }

        //docsig stuff
        int GetSignatureWidth() const;
        int GetSignatureMinLength() const;
        bool ShouldCalculateSignature() const;
        bool ShouldReturnDocStaticSig() const;
        i64 DocStaticSig() const;

        inline int Hack2FragMax() const {
            return 20;
        }

        //losswords stuff
        bool LossWords() const;
        bool LossWordsTitle() const;
        bool LossWordsDump() const;
        bool LossWordsExact() const;

        // Issue: SNIPPETS-1454
        float ShortMultifragCoef() const;

        int GetYandexWidth() const;
        bool UseTurkey() const;

        // Issue: SNIPPETS-4561
        const TString& GetISnipDataKeys() const;

        // SNIPPETS-5377
        size_t GetMinSymbolsBeforeHyphenation() const;
        size_t GetMinWordLengthForHyphenation() const;

        // FACTS-504
        float GetSymbolCutFragmentWidth() const;

        // SNIPPETS-1864
        bool DisableOgTextReplacer() const;

        bool CalculatePLM() const;

        // SNIPPETS-2061
        bool UseStrictHeadersOnly() const;

        // SNIPPETS-2117
        bool DefIdf() const;

        // SNIPPETS-2119
        bool SynCount() const;

        // SNIPPETS-2199
        bool AlmostUserWordsInTitles() const;
        bool UseAlmostUserWords() const;

        // SnipExp markers stuff
        TString GetExpsForMarkers() const;
        bool UseSnipExpMarkers() const;
        bool SnipExpProd() const;
        int GetDiffRatio() const;
        bool SwapWithProd() const;
        bool ProdLinesCount() const;
        bool TitlesDiffOnly() const;

        // SNIPPETS-2269
        bool SuppressDmozTitles() const;

        ui32 GetHitsTopLength() const;

        // SNIPPETS-2212
        int GetExtRatio() const;

        bool IgnoreDuplicateExtensions() const;

        // SNIPPETS-2332
        bool GenerateUrlBasedTitles() const;

        // SNIPPETS-2210
        int AltSnippetCount() const;
        // 0 is for the base snippet, 1..MAX_ALT_SNIPPETS are for additional snippets
        TString AltSnippetExps(int index) const;

        // SNIPPETS-2375
        bool TitlesWithSplittedHostnames() const;

        // SNIPPETS-2442
        bool UseMContentBoost() const;

        // SNIPPETS-2502
        bool ForumsInTitles() const;

        // SNIPPETS-2599
        bool CatalogsInTitles() const;

        // SNIPPETS-2505
        bool SwitchOffReplacer(const TString& replacerName) const;

        // SNIPPETS-3917 et al
        bool SwitchOffExtendedReplacerAnswer(const TString& replacerName) const;

        // SNIPPETS-2239
        bool BuildSahibindenTitles() const;

        // SNIPPETS-2393
        bool ForumForPreview() const;

        // SNIPPETS-2444
        bool TrashPessTr() const;

        // SNIPPETS-2494
        bool UrlmenuInTitles() const;

        // SNIPPETS-2524
        bool UseSchemaVideoObj() const;

        // SNIPPETS-2567
        int HostNameForUrlTitles() const;

        // SNIPPETS-2074
        bool DefinitionForNewsTitles() const;

        // title generating stuff
        ETitleGeneratingAlgo GetTitleGeneratingAlgo() const;
        ETitleDefinitionMode GetDefinitionMode() const;
        float DefinitionPopularityThreshold() const; // SNIPPETS-3478
        bool TitlesReadabilityExp() const; // SNIPPETS-2448
        bool EliminateDefinitions() const; // SNIPPETS-7265
        bool IsTitleContractionOk(size_t originalTitleLen, size_t newTitleLen) const; // SNIPPETS-7265

        // SNIPPETS-2609
        bool BNASnippets() const;

        // SNIPPETS-2619
        bool GetMainBna() const;
        bool GetBigDescrForMainBna() const;
        ui32 GetMaxBnaSnipRowCount() const; // SNIPPETS-5993
        ui32 GetMaxBnaRealSnipRowCount() const; // SNIPPETS-5993

        // SNIPPETS-2535
        bool UseRobotsTxtStub() const;
        bool RobotsTxtSkipNPS() const;

        bool RTYSnippets() const;

        // SNIPPETS-2610
        bool MetaDescriptionsInTitles() const;

        // SNIPPETS-1751
        bool CapitalizeEachWordTitleLetter() const;

        // SNIPPETS-2533
        bool AddPopularHostNameToTitle() const;
        float GetPopularHostRank() const;
        EDelimiterType GetTitlePartDelimiterType() const;
        bool GetRankingFactor(const size_t& factorId, float& value) const;
        const float* GetRankingFactors() const;
        size_t GetRankingFactorsCount() const;

        // SNIPPETS-2667
        bool TwoLineTitles() const;

        // SNIPPETS-2693
        bool UrlRegionsInTitles() const;
        bool UserRegionsInTitles() const;
        TString GetRelevRegionName() const;
        bool MnRelevRegion() const;
        float MnRelevRegionThreshold() const;

        // SNIPPETS-2713
        bool TunedForumClassifier() const;

        // SNIPPETS-2749
        // SNIPPETS-1874
        bool ShuffleWords() const;
        bool BrkBold() const;
        bool BrkReg() const;
        bool BrkTit() const;
        bool BrkGg() const;
        size_t BrkPct() const;
        size_t BrkProb() const;

        // SNIPPETS-2808
        float GetByLinkLen() const;

        // SNIPPETS-2875
        bool OpenGraphInTitles() const;

        bool UseTurkishReadabilityThresholds() const;

        // SNIPPETS-2923
        bool UseExtSnipRowLimit() const;
        ui32 GetExtSnipRowLimit() const;

        // SNIPPETS-2933
        bool EraseBadSymbolsFromTitle() const;

        const TString& GetReport() const;
        bool IsWebReport() const;

        bool AddGenericSnipForMobile() const;

        bool QuotesInYaca() const;
        bool ShortYacaTitles() const;
        size_t ShortYacaTitleLen() const;

        // SNIPPETS-3120
        bool SuppressCyrForSpok() const;

        // SNIPPETS-5345
        size_t AllowedTitleEmojiCount() const;
        size_t AllowedSnippetEmojiCount() const;

        // BLNDR-5599
        size_t AllowedExtSnippetEmojiCount() const;
        bool ShouldCanonizeUnicode() const;

        // SNIPPETS-3429
        bool IsRelevantQuestionAnswer() const;
        bool QuestionTitles() const;
        bool QuestionTitlesApproved() const;
        bool QuestionSnippets() const;
        bool QuestionSnippetsRelevant() const;
        float QuestionSnippetLen() const;

        bool BigMediawikiSnippet() const;
        float BigMediawikiSnippetLen() const;

        // FACTS-1002
        float BigMediawikiReadMoreLenMultiplier() const;
        // FACTS-920
        float BigMediawikiQualityFilterMultiplier() const;

        // SNIPPETS-3037
        bool UseYoutubeChannelImage() const;

        // SNIPPETS-2955
        bool IsTouchReport() const;
        float GetRowScale() const;

        // SNIPPETS-3242
        bool IsPadReport() const;

        // SNIPPETS-3227
        bool UseMobileUrl() const;
        bool HideUrlMenuForMobileUrl() const;

        bool UseProductOfferImage() const;

        // SNIPPETS-3454
        bool ChooseLongestSnippet() const;

        bool IsCommercialQuery() const;
        bool SkipTouchSnippets() const;
        bool UseTouchSnippetBody() const;
        bool UseTouchSnippetTitle() const;

        bool ExpFlagOn(const TString& flag) const;
        bool ExpFlagOff(const TString& flag) const;

        // Factchecking queries answering stuff
        const TAnswerModels* GetAnswerModels() const;
        const THostStats* GetHostStats() const;
        const NNeuralNetApplier::TModel* GetRuFactSnippetDssmApplier() const;
        const NNeuralNetApplier::TModel* GetTomatoDssmApplier() const;

        // Relev params
        using TRelevParams = THashMap<TString, TString>;
        const TRelevParams& GetRelevParams() const;

        // SNIPPETS-5808
        bool TopCandidatesRequested() const;
        size_t TopCandidateCount() const;
        size_t FactSnipTopCandidateCount() const;
        bool SkipAlgoInTopCandidateDump(const TString& algoName) const;
        const TFactorsToDump& TopCandidateFactorsToDump() const;

        // SNIPPETS-7241
        ui32 GetFormulaDegradeThreshold() const;

        // SNIPPETS-7243, experiment to degrade snippets
        ui32 GetAppendRandomWordsCount() const;

        // SNIPPETS-8196
        bool IsEU() const;
        bool SuppressCyr() const;
        bool UseMediawiki() const;

        // SNIPPETS-8210
        size_t ForceCutSnip() const;

        // BLNDR-6145
        ui32 CheckPassageRepeatsTitleAlgo() const;
        float GetPassageRepeatsTitlePessimizeFactor() const;

        //SNIPPETS-8827
        bool AllowBreveInTitle() const;
    };
}
