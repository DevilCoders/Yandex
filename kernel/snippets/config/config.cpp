#include "config.h"
#include "enums.h"

#include <kernel/snippets/factors/factors.h>
#include <kernel/snippets/formulae/all.h>
#include <kernel/snippets/formulae/manual.h>
#include <kernel/snippets/formulae/manual_img.h>
#include <kernel/snippets/formulae/manual_video.h>
#include <kernel/snippets/formulae/formula.h>

#include <kernel/snippets/idl/snippets.pb.h>
#include <search/idl/events.ev.pb.h>

#include <kernel/relevfml/rank_models_factory.h>
#include <kernel/factor_storage/factors_reader.h>
#include <kernel/web_factors_info/factors_gen.h>

#include <library/cpp/langmask/serialization/langmask.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/stopwords/stopwords.h>
#include <library/cpp/eventlog/eventlog.h>
#include <library/cpp/codecs/float_huffman.h>

#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <util/string/type.h>
#include <util/system/defaults.h>
#include <util/generic/singleton.h>

namespace NSnippets {

static constexpr int MAX_ALT_SNIPPETS = 2;
static constexpr float MINIMAL_SNIPPET_ROW_LEN = 0.1f;
static constexpr int DEFAULT_SNIP_WIDTH = 607;
static constexpr float DEFAULT_SYMBOL_CUT_FRAGMENT_WIDTH = 300.0f;

void TRankingFactors::Load(const NProto::TSnipReqParams& in) {
    if (in.UncompressedFactorsSize()) {
        FactorsRaw.assign(in.GetUncompressedFactors().begin(), in.GetUncompressedFactors().end());
        FactorsView.Reset(new ::TFactorBuf(FactorsRaw.size(), FactorsRaw.data()));
        if (in.HasFactorsBorders()) {
            TString factorBordersStr;
            Base64Decode(in.GetFactorsBorders(), factorBordersStr);
            NFactorSlices::TFactorBorders borders;
            DeserializeFactorBorders(factorBordersStr, borders);
            FactorDomain = TFactorDomain(borders);
            Y_ENSURE(FactorDomain.Size() == FactorsRaw.size());
            FactorsView->SetDomain(FactorDomain);
        }
    } else if (in.HasCompressedFactors()) {
        TString compressedFactorsStr;
        Base64Decode(in.GetCompressedFactors(), compressedFactorsStr);

        if (in.HasFactorsBorders()) {
            TString factorBordersStr;
            Base64Decode(in.GetFactorsBorders(), factorBordersStr);
            try {
                THolder<NFSSaveLoad::TFactorsReader> reader =
                    NFSSaveLoad::CreateHuffmanReader(factorBordersStr, compressedFactorsStr);
                reader->ReadTo(FactorsRaw);

                TFactorBorders borders;
                reader->PrepareModifiedBorders(borders);
                FactorDomain = TFactorDomain(borders);
            } catch (NFSSaveLoad::TParseFailError&) {
                ythrow yexception() << "invalid factor borders, failed to parse: " << factorBordersStr;
            } catch (NFSSaveLoad::TBorderValuesError&) {
                ythrow yexception() << "invalid factor borders, incorrect border values: " << factorBordersStr;
            }
        } else {
            FactorsRaw = NCodecs::NFloatHuff::Decode(compressedFactorsStr);
        }

        FactorsView.Reset(new ::TFactorBuf(FactorsRaw.size(), FactorsRaw.data()));
        if (in.HasFactorsBorders()) {
            FactorsView->SetDomain(FactorDomain);
        }
    }
}

const float* TRankingFactors::GetRawFactors() const {
    return FactorsView->GetRawFactors();
}

float TRankingFactors::operator[](size_t i) const {
    if (FactorsView.Get())
        return (*FactorsView)[i];
    return 0;
}

size_t TRankingFactors::Size() const {
    if (FactorsView.Get())
        return FactorsView->Size();
    return 0;
}


struct TSnipReqParams {
    i32 ExtendSnippets = 0;
    i32 MaxTitleLength = 60;
    TSnipInfoReqParams InfoReqParams = TSnipInfoReqParams();
    TLangMask QueryLangMask;
    i32 MaxSnippetCount = MAX_SNIP_COUNT;
    bool AbsentT = false;
    bool AllPassageHits = false;
    bool PaintedRawPassages = false;
    bool MarkParagraphs = false;
    bool AllowYacaSnippets = true;
    int RequestedSnippetLength = 0;
    bool IsMobileSearch = false;
    bool IsTelOrg = false;
    bool IsUrlOrEmailQuery = false;
    int SnippetsAltHeader = -1;
    EFaceType FaceType = ftDefault;
    i32 MaxTitleStartLen = 30;
    i32 MaxTitlePixelLen = 0;
    bool RawPassages = false;
    bool DumpCandidates = false;
    bool DumpAllAlgo3Pairs = false;
    bool DumpWithUnusedFactors = false;
    bool DumpWithManualFactors = false;
    bool DumpForPoolGeneration = false;
    bool SlovariSearch = false;
    bool MainPage = false;
    TString SnippetExperiments = TString();
    ELanguage ForeignNavHackLang = LANG_UNK;
    ETextCutMethod TitleCutMethod = TCM_PIXEL;
    ELanguage UILangCode = LANG_UNK;
    TString TLD = TString();
    i32 SignatureWidth = 4;
    i32 SignatureMinLength = 6;
    bool ShouldCalculateSignature = false;
    bool ShouldReturnDocStaticSig = false;
    i64 DocStaticSig = 0;
    bool IsPolitician = false;
    ETextCutMethod SnippetCutMethod = TCM_PIXEL;
    float RequestedSnippetLengthRow = 0.0;
    bool VideoWatch = false;
    bool IsBuildForImg = false;
    bool IsBuildForVideo = false;
    bool UseStaticDataOnly = false;
    TString StaticDataSource = TString();
    bool PaintNoAttrs = false;
    bool ForceStann = false;
    int FStannLen = 0;
    int FStannTitleLen = 0;
    bool UnpackFirst10 = false;
    bool IsTurkey = false;
    ui64 RelevRegion = 0;
    NSc::TValue EntityData = NSc::TValue();
    TVector<TString> SnipParamsForDebug = TVector<TString>();
    ui32 HitsTopLength = 30;
    bool ForcePureTitle = false;
    bool IgnoreRegionPhrase = false;
    i32 SnipWidth = DEFAULT_SNIP_WIDTH;
    bool SnipWidthWasSet = false;
    bool RTYSnippets = false;
    ui32 MaxUrlLength = 50;
    ui32 UrlCutThreshold = 40;
    ui32 MaxUrlmenuLength = 76;
    ui64 UserReqHash = 0;
    TRankingFactors RankingFactors;
    TString RelevRegionName = TString();
    bool MainBna = false;
    TString Report = TString();
    TConfig::TRelevParams RelevParams;

    bool GetRankingFactor(const size_t& factorId, float& value) const;
    const float* GetRankingFactors() const;
    size_t GetRankingFactorsCount() const;

    void Load(const NProto::TSnipReqParams& in);
private:
    void SetDefaults() {
        *this = TSnipReqParams();
    }
};

struct TExps {
    bool Extend = false;
    bool ExtendDiff = false;
    bool UseDiv2 = true;
    size_t Algo3TopLen = 20;
    bool DumpCands = false;
    bool DumpAllPairs = false;
    int DumpMode = 0;
    TFactorsToDump FactorsToDump;
    TFactorsToDump TopCandidateFactorsToDump;
    bool FinalDump = false;
    bool FinalDumpBinary = false;
    bool Hits1 = false;
    bool Hits2 = false;
    bool Hits3 = false;
    bool Hits4 = false;
    bool Hits5 = false;
    bool Hits6 = false;
    bool MarkPar = false;
    bool Foo = false;
    bool Bar = false;
    bool Baz = false;
    bool Qux = false;
    bool Fred = false;
    bool WeightedReplacers = true;
    bool LossWords = false;
    bool LossWordsTitle = true;
    bool LossWordsDump = false;
    bool LossWordsExact = false;
    bool CatalogOptOutAllowed = true;
    TString Redump = "";
    TRetexts Retexts = TRetexts();
    bool IsBuildForImg = false;
    bool IsBuildForVideo = false;
    bool ErasePoorAlts = true;
    bool UnpAll = false;
    bool RawAll = false;
    bool AllowCyrForTr = false;
    bool VideoBlock = false;
    bool ForceVideoBlock = false;
    bool ForceStann = false;
    bool NoStann = false;
    bool PreviewSegmentsOff = false;
    bool PreviewSchema = false;
    bool AllowVthumbPreview = false;
    bool MaskBaseImagesPreview = false;
    bool DropBaseImagesPreview = false;
    int FStannLen = 0;
    int FStannTitleLen = 0;
    bool BypassReplacers = false;
    bool DumpManual = false;
    bool DumpUnused = false;
    bool DumpForPool = false;
    ELanguage ForceUILangCode = LANG_UNK;
    size_t YandexWidth = 0;
    bool UseAllMarksBoost = false;
    bool UseAllMPairwiseBoost = false;
    bool UseNewMarksBoost = false;
    bool UseActiveLearning = false;
    bool UseActiveLearningBoost = false;
    bool UseRdotsBoostLiteTR = false;
    bool UseRdotsBoostTR = false;
    bool UseActiveLearningBoostTR = false;
    bool UseRandomWithoutBoost = false;
    bool UseRandomWithBoost = false;
    bool UseActiveLearningInformativeness = false;
    bool WithPLMLikeBoostInTurkey = false;
    bool WithoutPLMLikeBoost = false;
    bool UseStrictHeadersOnly = false;
    bool DisableOgTextReplacer = false;
    bool IsVideoExp = false;
    bool UseListSnip = true;
    bool DropListStat = false;
    bool DropListHeader = false;
    bool UseTableSnip = false;
    bool DropTallTables = false;
    bool DropBrokenTables = false;
    bool DropTableStat = false;
    bool DropTableHeader = false;
    bool ShrinkTableRows = false;
    bool NoDbg = false;
    bool DefIdf = true;
    bool SynCount = false;
    bool AlmostUserWords = false;
    bool LinesCountBoost = false;
    bool InfReadFormula = false;
    int ShareMessedSnip = 0;
    bool ShuffleHilite = false;
    bool NoHilite = false;
    bool AddHilite = false;
    bool SnipExpProd = false;
    TString ExpsForMarkers = "";
    bool SuppressDmozTitles = true;
    int DiffRatio = 100;
    int ExtRatio = 100;
    bool IgnoreDuplicateExtensions = false;
    bool AllowDuplicateExtensionsInTurkey = false;
    bool CyrPessimization = true;
    bool SwapWithProd = false;
    bool ProdLinesCount = false;
    bool GenerateUrlBasedTitles = true;
    int AltSnippetCount = 0;
    TVector<TString> AltSnippetExps = TVector<TString>(MAX_ALT_SNIPPETS + 1);
    bool TitlesWithoutSplittedHostnames = false;
    bool NeedFormRawPreview = false;
    bool Formula28883 = false;
    bool Formula28185 = false;
    bool Formula1938 = false;
    bool Formula2121 = false;
    bool Formula1940 = false;
    bool Formula29410 = false;
    bool FormulaSnip1Click10k = false;
    bool FormulaSnip1Click20k = false;
    bool FormulaSnip2Click10k = false;
    bool FormulaSnip2Click20k = false;
    bool FormulaSnip3NoClick10k = false;
    bool FormulaSnip3NoClick20k = false;
    bool ReverRusTrash = false;
    bool Snippets2470a = false;
    bool UseMContentBoost = true;
    bool ForumsInTitles = true;
    bool CatalogsInTitles = true;
    THashSet<TString> SwitchedOffReplacers = THashSet<TString>();
    THashSet<TString> SwitchedOffExtendedReplacers = THashSet<TString>();
    bool BuildSahibindenTitles = true;
    bool UseBadSegments = false;
    bool TrashPessTr = false;
    bool UrlmenuInTitles = true;
    bool UseSchemaVideoObj = true;
    int HostNameForUrlTitles = 0;
    bool DefinitionForNewsTitles = true;
    bool ForumPreview = false;
    bool BNASnippets = false;
    size_t BNASitelinksWidth = 235;
    bool UseRobotsTxtStub = true;
    bool RobotsTxtSkipNPS = true;
    bool MetaDescriptionsInTitles = true;
    bool CapitalizeEachWordTitleLetter = true;
    bool AddPopularHostNameToTitle = false;
    float PopularHostRank = 0.8f;
    EDelimiterType TitlePartDelimiterType = DELIM_PIPE;
    bool TwoLineTitles = false;
    bool UseStaticDataOnly = false;
    bool UrlRegionsInTitles = false;
    bool UserRegionsInTitles = true;
    bool MnRelevRegion = false;
    float MnRelevRegionThreshold = 0.5f;
    bool TunedForumClassifier = true;
    bool Snippets1874 = false;
    bool KpBold = false;
    bool KpReg = false;
    bool KpTit = false;
    bool KpGg = false;
    size_t KpPct = 0;
    size_t KpProb = 0;
    int TitleWidthReserve = 21;
    bool UnpaintTitle = false;
    int RequestedSnippetLength = 0;
    float CommercialSnippetLengthRow = 0.f;
    float RequestedSnippetLengthRow = 0.f;
    float RequestedPadSnippetLengthRow = 2.0f;
    TAtomicSharedPtr<TMxNetFormula> DynamicFormula;
    TAtomicSharedPtr<TMxNetFormula> DefaultDynamicFormula;
    TAtomicSharedPtr<TMxNetFormula> DynamicFactSnippetFormula;
    TAtomicSharedPtr<TMxNetFormula> DefaultDynamicFactSnippetFormula;
    bool NoExtdw = false;
    ui32 HitsTopLength = 0;
    bool OpenGraphInTitles = true;
    float BNATitleFontSize = 18.0f;
    float TitleFontSize = 0.0f;
    float SnipFontSize = 0.0f;
    bool ReverseTitlesExp = false;
    bool TitlesDiffOnly = false;
    bool MainBna = false;
    bool TitlesReadabilityExp = true;
    ETitleGeneratingAlgo TitleGeneratingAlgo = TGA_PREFIX;
    ETitleDefinitionMode DefinitionMode = TDM_USE;
    bool EliminateDefinitions = false;
    float TitleContractionRatioThreshold = 0.25f;
    size_t TitleContractionSymbolThreshold = 40;
    float DefinitionPopularityThreshold = -1.0f;
    bool UseExtSnipRowLimit = false;
    bool UseTouchSnippetTitle = true;
    bool UseTouchSnippetBody = true;
    ui32 ExtSnipRowLimit = 12;
    float TouchRowScale = 1.13f;
    float PadRowScale = 0.78f;
    ui32 TitleRowLimit = 0;
    float TitleBnaRowLimit = 0.0f;
    ui32 TouchTitleRowLimit = 3;
    bool EraseBadSymbolsFromTitle = true;
    TString Report = TString();
    bool BigDescrForMainBna = true;
    bool GenericForMobile = false;
    bool QuotesInYaca = true;
    bool ShortYacaTitles = false;
    size_t ShortYacaTitleLen = 30;
    bool SuppressCyrForSpok = true;
    size_t AllowedTitleEmojiCount = 0;
    size_t AllowedSnippetEmojiCount = 0;
    size_t AllowedExtSnippetEmojiCount = 0;
    bool ShouldCanonizeUnicode = true;
    bool QuestionTitles = true;
    bool QuestionTitlesApproved = false;
    bool QuestionTitlesRelevant = false;
    bool QuestionSnippetsApproved = false;
    bool QuestionSnippetsRelevant = false;
    float QuestionSnippetLen = 6.0f;
    bool BigMediawikiSnippet = false;
    float BigMediawikiSnippetLen = 3.0f;
    float BigMediawikiTouchSnippetLen = 6.0f;
    float BigMediawikiReadMoreLenMultiplier = 3.0f;
    float BigMediawikiQualityFilterMultiplier = 2.0f;
    bool UseYoutubeChannelImage = false;
    bool UsePadSnippets = true;
    bool UseMobileUrl = true;
    bool HideUrlMenuForMobileUrl = false;
    bool LinksInSnip = false;
    bool UseProductOfferImage = true;
    size_t MaxSnipCount = 0;
    bool ChooseLongestSnippet = false;
    bool FilterCommercialQueries = false;
    THashSet<TString> FlagsOn;
    THashSet<TString> FlagsOff;
    ui32 MobileHilitedUrlReduction = 0;
    float SymbolCutFragmentWidth = DEFAULT_SYMBOL_CUT_FRAGMENT_WIDTH;
    int SnipWidth = 0;
    TString ISnipDataKeys;
    size_t MinSymbolsBeforeHyphenation = 0;
    size_t MinWordLengthForHyphenation = 0;
    size_t TopCandidateCount = 0;
    size_t FactSnipTopCandidateCount = 0;
    THashSet<TString> TopCandidateSkippedAlgos, TopCandidateTakenAlgos;
    ui32 MaxBnaSnipRowCount = 0;
    ui32 MaxBnaRealSnipRowCount = 0;
    TVector<std::pair<size_t, double>> FactorBoost;
    TVector<std::pair<size_t, double>> FactSnippetFactorBoost;
    THashMap<TString, double> Report2RealLenBoost;
    TArrayHolder<double> DynamicManualCoeffs;
    THolder<TLinearFormula> DynamicManualBoost;
    TArrayHolder<double> FactSnippetDynamicManualCoeffs;
    THolder<TLinearFormula> FactSnippetDynamicManualBoost;
    ui32 FormulaDegradeThreshold = 0;
    ui32 AppendRandomWordsCount = 0;
    size_t ForceCutSnip = 0;
    TString GreenUrlDomainTrieKey = "";
    ui32 CheckPassageRepeatsTitleAlgo = 0;
    float PassageRepeatsTitlePessimizeFactor = 0.0;
    bool AllowBreveInTitle = false;
    double BagOfWordsIntersectionThreshold = 0.8;

    void Reset() {
        *this = TExps();
    };
};

struct TInfoReqParamsDbgSet {
    TSnipInfoReqParams                   InfoReqParamsDbg1;
    TSnipInfoReqParams                   InfoReqParamsDbg2;
    TSnipInfoReqParams                   InfoReqParamsDbg3;
    TSnipInfoReqParams                   InfoReqParamsDbg4;
    TSnipInfoReqParams                   InfoReqParamsDbg5;
    TSnipInfoReqParams                   InfoReqParamsDbg6;

    TInfoReqParamsDbgSet() {
        InfoReqParamsDbg1.PageNumber = 1;
        InfoReqParamsDbg1.RequestType = INFO_SNIPPET_HITS;
        InfoReqParamsDbg1.UnpackedOnly = true;
        InfoReqParamsDbg1.ShowMarkup = true;
        InfoReqParamsDbg1.IsLinkArchive = false;
        InfoReqParamsDbg1.TableType = INFO_XML;
        InfoReqParamsDbg1.OnlySkipRestr = false;
        InfoReqParamsDbg1.Unpaged = true;
        InfoReqParamsDbg2 = InfoReqParamsDbg1;
        InfoReqParamsDbg2.UnpackedOnly = false;
        InfoReqParamsDbg3 = InfoReqParamsDbg1;
        InfoReqParamsDbg3.RequestType = INFO_SNIPPETS;
        InfoReqParamsDbg3.AlgoName = "_dbg";
        InfoReqParamsDbg4 = InfoReqParamsDbg1;
        InfoReqParamsDbg4.TableType = INFO_TABSEP;
        InfoReqParamsDbg4.ShowMarkup = false;
        InfoReqParamsDbg4.OnlySkipRestr = true;
        InfoReqParamsDbg5 = InfoReqParamsDbg1;
        InfoReqParamsDbg5.TableType = INFO_JSON;
        InfoReqParamsDbg5.ShowMarkup = false;
        InfoReqParamsDbg5.OnlySkipRestr = true;
        InfoReqParamsDbg6 = InfoReqParamsDbg1;
        InfoReqParamsDbg6.UnpackedOnly = false;
        InfoReqParamsDbg6.TableType = INFO_TABSEP;
        InfoReqParamsDbg6.ShowMarkup = false;
        InfoReqParamsDbg6.OnlySkipRestr = true;
    }
};

struct TConfig::TImpl {
    TSnipReqParams RP;
    const TWordFilter*                   StopWords = nullptr;
    const ISentsFilter*                  SentsFilter = nullptr;
    const IGeobase*                      Geobase = nullptr;
    const IRankModelsFactory*            DynamicModels = nullptr;
    const TAnswerModels*                 AnswerModels = nullptr;
    const THostStats*                    HostStats = nullptr;

    const NNeuralNetApplier::TModel*     RuFactSnippetDssmApplier = nullptr;
    const NNeuralNetApplier::TModel*     TomatoDssmApplier = nullptr;

    TEventLogFrame*                      EventLog = nullptr;
    ui32                                 DocId = 0;
    TString                               ReqId;
    TExps                                Exps;

    explicit TImpl(const TConfigParams& params);

    void LoadRP(const NProto::TSnipReqParams& in);
    void AppendExps(const TStringBuf& exps);
};

TConfig::TImpl::TImpl(const TConfigParams& params)
    : RP()
    , StopWords(params.StopWords ? params.StopWords : &TWordFilter::EmptyFilter)
    , SentsFilter(params.SentsFilter)
    , Geobase(params.Geobase)
    , DynamicModels(params.DynamicModels)
    , AnswerModels(params.AnswerModels)
    , HostStats(params.HostStats)
    , RuFactSnippetDssmApplier(params.RuFactSnippetDssmApplier)
    , TomatoDssmApplier(params.TomatoDssmApplier)
    , EventLog(params.EventLog)
    , DocId(params.DocId)
    , ReqId(params.ReqId)
    , Exps()
{
    if (params.SRP) {
        LoadRP(*params.SRP);
    }
    if (!!params.DefaultExps) {
        AppendExps(params.DefaultExps);
    }
    for (const TString& exp : params.AppendExps) {
        AppendExps(exp);
    }
}

static void InitDynamicManualCoeffs(const TLinearFormula& base, const TVector<std::pair<size_t, double>>& factorBoost,
    const double* lenBoost, TArrayHolder<double>& dynamicManualCoeffs, TString& formulaName)
{
    dynamicManualCoeffs.Reset(new double[base.GetFactorCount()]);
    for (size_t i = 0; i < base.GetFactorCount(); i++) {
        dynamicManualCoeffs[i] = base.GetKoef(i);
    }
    if (lenBoost) {
        Y_ASSERT(base.GetFactorCount() > A2_REAL_LEN);
        dynamicManualCoeffs[A2_REAL_LEN] += *lenBoost;
        formulaName += ":len_boost=" + FloatToString(*lenBoost);
    }
    for (auto [num, val] : factorBoost) {
        Y_ASSERT(base.GetFactorCount() > num);
        formulaName += ":factor" + ToString<int>(num) + "_boost=" + FloatToString(val);
        dynamicManualCoeffs[num] += val;
    }
}

TConfig::TConfig(const TConfigParams& params)
  : Impl(new TImpl(params))
{
    double* lenBoost = Impl->Exps.Report2RealLenBoost.FindPtr(GetReport());
    if (lenBoost != nullptr || !Impl->Exps.FactorBoost.empty()) {
        const TLinearFormula& base = GetManual();
        TString formulaName = base.GetName();
        InitDynamicManualCoeffs(base, Impl->Exps.FactorBoost, lenBoost, Impl->Exps.DynamicManualCoeffs, formulaName);
        Impl->Exps.DynamicManualBoost =
                MakeHolder<TLinearFormula>(formulaName, base.GetFactorDomain(), Impl->Exps.DynamicManualCoeffs.Get());
    }
    if (!Impl->Exps.FactSnippetFactorBoost.empty()) {
        const TLinearFormula& base = GetFactSnippetManual();
        TString formulaName = base.GetName();
        InitDynamicManualCoeffs(base, Impl->Exps.FactSnippetFactorBoost, nullptr, Impl->Exps.FactSnippetDynamicManualCoeffs, formulaName);
        Impl->Exps.FactSnippetDynamicManualBoost =
                MakeHolder<TLinearFormula>(formulaName, base.GetFactorDomain(), Impl->Exps.FactSnippetDynamicManualCoeffs.Get());
    }
}

TConfig::~TConfig() {
}

ui32 TConfig::GetDocId() const {
    return Impl->DocId;
}

TString TConfig::GetSnipParamsForDebug() const {
    constexpr TStringBuf REGPHRASE_PARAM = "regphrase=";
    constexpr TStringBuf MOREHL_PARAM = "morehl=";
    const char SEPARATOR = ';';
    TStringBuilder snipDebug;
    for (const TString& params : Impl->RP.SnipParamsForDebug) {
        TStringBuf paramsBuf(params);
        while (paramsBuf) {
            TStringBuf param = paramsBuf.NextTok(SEPARATOR);
            // don't set long and unreadable richtrees
            if (param.StartsWith(REGPHRASE_PARAM)) {
                snipDebug << REGPHRASE_PARAM;
            } else if (param.StartsWith(MOREHL_PARAM)) {
                snipDebug << MOREHL_PARAM;
            } else {
                snipDebug << param;
            }
            snipDebug << SEPARATOR;
        }
    }
    return snipDebug;
}

const TWordFilter& TConfig::GetStopWordsFilter() const {
    return *Impl->StopWords;
}

const ISentsFilter* TConfig::GetSentsFilter() const {
    return Impl->SentsFilter;
}

const IGeobase* TConfig::GetGeobase() const {
    return Impl->Geobase;
}

// Issue: SNIPPETS-1454
float TConfig::ShortMultifragCoef() const {
    return 0.97f;
}

void TSnipReqParams::Load(const NProto::TSnipReqParams& in) {
    SetDefaults();
    if (in.HasExtendSnippets()) {
        ExtendSnippets = in.GetExtendSnippets();
    }
    if (in.HasMaxTitleLength()) {
        MaxTitleLength = in.GetMaxTitleLength();
    }
    if (in.HasInfoRequestParams()) {
        const NProto::TSnipInfoReqParams& p = in.GetInfoRequestParams();
        if (p.HasPageNumber()) {
            InfoReqParams.PageNumber = p.GetPageNumber();
        }
        if (p.HasUnpackedOnly()) {
            InfoReqParams.UnpackedOnly = p.GetUnpackedOnly();
        }
        if (p.HasInfoRequestType()) {
            InfoReqParams.RequestType = (EInfoRequestType)p.GetInfoRequestType();
        }
        if (p.HasShowMarkup()) {
            InfoReqParams.ShowMarkup = p.GetShowMarkup();
        }
        if (p.HasIsLinkArchive()) {
            InfoReqParams.IsLinkArchive = p.GetIsLinkArchive();
        }
        if (p.HasAlgoName()) {
            InfoReqParams.AlgoName = p.GetAlgoName();
        }
        if (p.HasIsJson()) {
            InfoReqParams.TableType = p.GetIsJson() ? INFO_JSON : INFO_XML;
        }
    }
    if (in.HasQueryLangMask()) {
        QueryLangMask = ::Deserialize(in.GetQueryLangMask());
    } else if (in.HasQueryLangMaskLow()) {
        QueryLangMask = TLangMask(in.GetQueryLangMaskLow()); // obsolete, only low 64 bits!
    }
    if (in.HasMaxSnippetCount()) {
        MaxSnippetCount = in.GetMaxSnippetCount();
    }
    if (in.HasAbsentT()) {
        AbsentT = in.GetAbsentT();
    }
    if (in.HasAllPassageHits()) {
        AllPassageHits = in.GetAllPassageHits();
    }
    if (in.HasPaintedRawPassages()) {
        PaintedRawPassages = in.GetPaintedRawPassages();
    }
    if (in.HasMarkParagraphs()) {
        MarkParagraphs = in.GetMarkParagraphs();
    }
    if (in.HasAllowYacaSnippets()) {
        AllowYacaSnippets = in.GetAllowYacaSnippets();
    }
    if (in.HasRequestedSnippetLength()) {
        RequestedSnippetLength = in.GetRequestedSnippetLength();
    }
    if (in.HasRequestedSnippetLengthRow()) {
        RequestedSnippetLengthRow = in.GetRequestedSnippetLengthRow();
    }
    if (in.HasIsMobileSearch()) {
        IsMobileSearch = in.GetIsMobileSearch();
    }
    if (in.HasFaceType()) {
        FaceType = (EFaceType)in.GetFaceType();
    }
    if (in.HasMaxTitleStartLen()) {
        MaxTitleStartLen = in.GetMaxTitleStartLen();
    }
    if (in.HasMaxTitlePixelLen()) {
        MaxTitlePixelLen = in.GetMaxTitlePixelLen();
    }
    if (in.HasRawPassages()) {
        RawPassages = in.GetRawPassages();
    }
    if (in.HasDumpCandidates()) {
        DumpCandidates = in.GetDumpCandidates();
    }
    if (in.HasDumpAllAlgo3Pairs()) {
        DumpAllAlgo3Pairs = in.GetDumpAllAlgo3Pairs();
    }
    if (in.HasDumpWithUnusedFactors()) {
        DumpWithUnusedFactors = in.GetDumpWithUnusedFactors();
    }
    if (in.HasDumpWithManualFactors()) {
        DumpWithManualFactors = in.GetDumpWithManualFactors();
    }
    if (in.HasSlovariSearch()) {
        SlovariSearch = in.GetSlovariSearch();
    }
    if (in.HasMainPage()) {
        MainPage = in.GetMainPage();
    }
    if (in.HasSnippetExperiments()) {
        SnippetExperiments = in.GetSnippetExperiments();
    }
    if (in.HasForeignNavHackLang()) {
        ForeignNavHackLang = LanguageByName(in.GetForeignNavHackLang());
    }
    if (in.HasIsTelOrg()) {
        IsTelOrg = in.GetIsTelOrg();
    }
    if (in.HasIsUrlOrEmailQuery()) {
        IsUrlOrEmailQuery = in.GetIsUrlOrEmailQuery();
    }
    if (in.HasSnippetsAltHeader()) {
        SnippetsAltHeader = in.GetSnippetsAltHeader();
    }
    if (in.HasTitleCutMethod()) {
        TitleCutMethod = (ETextCutMethod)in.GetTitleCutMethod();
    }
    if (in.HasSnippetCutMethod()) {
        SnippetCutMethod = (ETextCutMethod)in.GetSnippetCutMethod();
    }
    if (in.HasUILang()) {
        UILangCode = LanguageByName(in.GetUILang());
    }
    if (in.HasTLD()) {
        TLD = in.GetTLD();
    }
    if (in.HasSignatureWidth()) {
        SignatureWidth = in.GetSignatureWidth();
    }
    if (in.HasSignatureMinLength()) {
        SignatureMinLength = in.GetSignatureMinLength();
    }
    if (in.HasShouldCalculateSignature()) {
        ShouldCalculateSignature = in.GetShouldCalculateSignature();
    }
    if (in.HasShouldReturnDocStaticSig()) {
        ShouldReturnDocStaticSig = in.GetShouldReturnDocStaticSig();
    }
    if (in.HasDocStaticSig()) {
        DocStaticSig = in.GetDocStaticSig();
    }
    if (in.HasIsPolitician()) {
        IsPolitician = in.GetIsPolitician();
    }
    if (in.HasVideoWatch()) {
        VideoWatch = in.GetVideoWatch();
    }
    if (in.HasImgsearchSnippets()) {
        IsBuildForImg = in.GetImgsearchSnippets();
    }
    if (in.HasVideosearchSnippets()) {
        IsBuildForVideo = in.GetVideosearchSnippets();
    }
    if (in.HasUseStaticDataOnly()) {
        UseStaticDataOnly = in.GetUseStaticDataOnly();
    }
    if (in.HasStaticDataSource()) {
        StaticDataSource = in.GetStaticDataSource();
    }
    if (in.HasPaintNoAttrs()) {
        PaintNoAttrs = in.GetPaintNoAttrs();
    }
    if (in.HasCPreview()) {
        ForceStann = in.GetCPreview();
    }
    if (in.HasCPreviewLength()) {
        FStannLen = in.GetCPreviewLength();
    }
    if (in.HasCPreviewTitleLength()) {
        FStannTitleLen = in.GetCPreviewTitleLength();
    }
    if (in.HasUnpackFirst10()) {
        UnpackFirst10 = in.GetUnpackFirst10();
    }
    if (in.HasRelevRegion()) {
        RelevRegion = in.GetRelevRegion();
    }
    if (in.HasIsTurkey()) {
        IsTurkey = in.GetIsTurkey();
    }
    if (in.HasEntityJson() && in.GetEntityJson().size()) {
        EntityData = NSc::TValue::FromJson(in.GetEntityJson());
    }
    for (size_t i = 0; i < in.SnipParamsForDebugSize(); ++i) {
        SnipParamsForDebug.push_back(in.GetSnipParamsForDebug(i));
    }
    if (in.HasHitsTopLength()) {
        HitsTopLength = in.GetHitsTopLength();
    }
    if (in.HasForcePureTitle()) {
        ForcePureTitle = in.GetForcePureTitle();
    }
    if (in.HasIgnoreRegionPhrase()) {
        IgnoreRegionPhrase = in.GetIgnoreRegionPhrase();
    }
    if (in.HasSnipWidth() && in.GetSnipWidth() >= 30) {
        SnipWidth = in.GetSnipWidth();
        SnipWidthWasSet = true;
    }
    if (in.HasRTYSnippets()) {
        RTYSnippets = in.GetRTYSnippets();
    }
    if (in.HasMaxUrlLength()) {
        MaxUrlLength = in.GetMaxUrlLength();
    }
    if (in.HasUrlCutThreshold()) {
        UrlCutThreshold = in.GetUrlCutThreshold();
    }
    if (in.HasMaxUrlmenuLength()) {
        MaxUrlmenuLength = in.GetMaxUrlmenuLength();
    }
    if (in.HasUserReqHash()) {
        UserReqHash = in.GetUserReqHash();
    }
    if (in.HasRelevRegionName()) {
        RelevRegionName = in.GetRelevRegionName();
    }
    if (in.HasMainBna()) {
        MainBna = in.GetMainBna();
    }
    if (in.HasReport()) {
        Report = in.GetReport();
    }
    RankingFactors.Load(in);
    for (const auto& param : in.GetRelevParams()) {
        RelevParams[param.GetName()] = param.GetValue();
    }
}

bool TSnipReqParams::GetRankingFactor(const size_t& factorId, float& value) const {
    if (factorId >= RankingFactors.Size()) {
        return false;
    }
    value = RankingFactors[factorId];
    return true;
}

const float* TSnipReqParams::GetRankingFactors() const {
    return RankingFactors.GetRawFactors();
}

size_t TSnipReqParams::GetRankingFactorsCount() const {
    return RankingFactors.Size();
}

struct TExpsConsumer {
    const IRankModelsFactory* DynamicModels = nullptr;
    TExps& Exps;
    TExpsConsumer(const IRankModelsFactory* models, TExps& exps)
      : DynamicModels(models)
      , Exps(exps)
    {
    }

    void ParseAltSnippetExps(const TStringBuf& key, const TStringBuf& value) {
        TStringBuf pureKey = key;
        if (pureKey.NextTok('_') != "alt") {
            return;
        }

        int snipIndex;
        if (!TryFromString(pureKey.NextTok('_'), snipIndex)) {
            return;
        }
        if (snipIndex < 0 || snipIndex > MAX_ALT_SNIPPETS || pureKey.empty()) {
            return;
        }

        TString& exps = Exps.AltSnippetExps.at(snipIndex);
        if (!!exps) {
            exps.append(",");
        }
        exps.append(pureKey);
        if (value.size()) {
            exps.append("=").append(value);
        }
    }

    bool IsTrueParam(const TString& value) {
        return value.empty() || IsTrue(value);
    }

    void AddFactorNums(TStringBuf ranges, TFactorsToDump& factorsToDump) {
        // ranges = "from1-to1:from2-to2:from3-to3...", single indices are also allowed
        while (ranges) {
            TStringBuf range = ranges.NextTok(':');
            TStringBuf fromStr = range.NextTok('-');
            TStringBuf toStr = (range ? range : fromStr);
            size_t from = FromStringWithDefault<size_t>(fromStr);
            size_t to = FromStringWithDefault<size_t>(toStr);
            factorsToDump.FactorRanges.emplace_back(from, to);
        }
    }

    const TFactorDomain& GetFactorDomainForSlices(const NMatrixnet::TMnSseInfo& formula) {
        using namespace NFactorSlices;
        const TVector<TString>& sliceNames = formula.GetSliceNames();
        const std::pair<EFactorSlice, const TFactorDomain&> allWebRankingSlices[] = {
            {EFactorSlice::SNIPPETS_WEBRANKING, algo2PlusWebDomain},
            {EFactorSlice::SNIPPETS_WEBRANKING_NOCLICK, algo2PlusWebNoClickDomain},
            {EFactorSlice::SNIPPETS_WEBRANKING_V1, algo2PlusWebV1Domain},
        };
        for (const TString& sliceName : sliceNames)
            for (auto& test : allWebRankingSlices)
                if (sliceName == ToString(test.first))
                    return test.second;
        return algo2Domain;
    }

    bool Consume(const char* b, const char* e, const char*) {
        const TStringBuf s(b, e);
        const TString str = ToString(s);
        if (str.size() >= 7 && str.substr(0, 7) == "marker_") {
            if (!Exps.ExpsForMarkers.empty())
                Exps.ExpsForMarkers.push_back(',');
            Exps.ExpsForMarkers += str.substr(7);
        }
        TString key = str;
        TString value;
        size_t eqPos = key.find_first_of('=');
        if (eqPos != TString::npos) {
            value = key.substr(eqPos + 1);
            key = key.substr(0, eqPos);
        }

        if (value.empty() || IsTrue(value)) {
            Exps.FlagsOn.insert(key);
        } else if (IsFalse(value)) {
            Exps.FlagsOff.insert(key);
        }

        if (key == "snip_width") {
            int width = FromStringWithDefault<int>(value);
            if (width > 0 && width < 1000) {
                Exps.SnipWidth = width;
            }
        } else if (key == "filter_commercial_queries") {
            Exps.FilterCommercialQueries = IsTrueParam(value);
        } else if (key == "choose_longest_snippet") {
            Exps.ChooseLongestSnippet = true;
        } else if (key == "t") {
            int num = FromStringWithDefault<int>(value);
            if (num > 0 && num <= 4) {
                Exps.MaxSnipCount = num;
            }
        } else if (key == "big_mediawiki_snippet_len") {
            float len = 0.0f;
            if (TryFromString(value, len) && len > 0.0f) {
                Exps.BigMediawikiSnippetLen = len;
            }
        } else if (key == "big_mediawiki_touch_snippet_len") {
            float len = 0.0f;
            if (TryFromString(value, len) && len > 0.0f) {
                Exps.BigMediawikiTouchSnippetLen = len;
            }
        } else if (key == "big_mediawiki_read_more_len_multiplier") {
            float len = 0.0f;
            if (TryFromString(value, len) && len > 0.0f) {
                Exps.BigMediawikiReadMoreLenMultiplier = len;
            }
        } else if (key == "big_mediawiki_quality_filter_multiplier") {
            float mult = 0.0f;
            if (TryFromString(value, mult) && mult > 0.0f) {
                Exps.BigMediawikiQualityFilterMultiplier = mult;
            }
        } else if (key == "question_titles") {
            Exps.QuestionTitles = IsTrueParam(value);
        } else if (key == "question_titles_approved") {
            Exps.QuestionTitlesApproved = IsTrueParam(value);
        } else if (key == "question_titles_relevant") {
            Exps.QuestionTitlesRelevant = IsTrueParam(value);
        } else if (key == "question_snippets_approved") {
            Exps.QuestionSnippetsApproved = IsTrueParam(value);
        } else if (key == "question_snippets_relevant") {
            Exps.QuestionSnippetsRelevant = IsTrueParam(value);
        } else if (key == "question_snippet_len") {
            float len = 0.0f;
            if (TryFromString(value, len) && len >= 0.0f) {
                Exps.QuestionSnippetLen = len;
            }
        } else if (key == "suppress_cyr_for_spok") {
            Exps.SuppressCyrForSpok = IsTrueParam(value);
        } else if (key == "allowed_title_emoji_count"){
            int cnt = FromStringWithDefault<int>(value);
            if (cnt >= 0) {
                Exps.AllowedTitleEmojiCount = cnt;
            }
        } else if (key == "allowed_snippet_emoji_count"){
            int cnt = FromStringWithDefault<int>(value);
            if (cnt >= 0) {
                Exps.AllowedSnippetEmojiCount = cnt;
            }
        } else if (key == "allowed_ext_snippet_emoji_count"){
            int cnt = FromStringWithDefault<int>(value);
            if (cnt >= 0) {
                Exps.AllowedExtSnippetEmojiCount = cnt;
            }
        } else if (key == "should_canonize_unicode"){
            Exps.ShouldCanonizeUnicode = IsTrueParam(value);
        } else if (key == "quotes_in_yaca") {
            Exps.QuotesInYaca = IsTrueParam(value);
        } else if (key == "short_yaca_titles") {
            Exps.ShortYacaTitles = IsTrueParam(value);
        } else if (key == "short_yaca_title_len") {
            int len = FromStringWithDefault<int>(value);
            if (len > 0 && len < 100) {
                Exps.ShortYacaTitleLen = len;
            }
        } else if (key == "bigdescr_formainbna") {
            Exps.BigDescrForMainBna = IsTrueParam(value);
        } else if (key == "erase_bad_symbols_from_title") {
            Exps.EraseBadSymbolsFromTitle = IsTrueParam(value);
        } else if (key == "title_generating_algo") {
            TryFromString(value, Exps.TitleGeneratingAlgo);
        } else if (key == "definition_mode") {
            TryFromString(value, Exps.DefinitionMode);
        } else if (key == "definition_popularity_threshold") {
            float threshold = 0.0f;
            if (TryFromString(value, threshold) &&
                threshold >= 0.0f && threshold <= 1.0f)
            {
                Exps.DefinitionPopularityThreshold = threshold;
            }
        } else if (key == "eliminate_definitions") {
            Exps.EliminateDefinitions = IsTrueParam(value);
        } else if (key == "title_contraction_ratio_threshold") {
            float threshold = 0.0f;
            if (TryFromString(value, threshold)) {
                Exps.TitleContractionRatioThreshold = threshold;
            }
        } else if (key == "title_contraction_symbol_threshold") {
            size_t threshold = 0.0f;
            if (TryFromString(value, threshold)) {
                Exps.TitleContractionSymbolThreshold = threshold;
            }
        } else if (key == "titles_readability_exp") {
            Exps.TitlesReadabilityExp = IsTrueParam(value);
        } else if (key == "titles_diff_only") {
            Exps.TitlesDiffOnly = IsTrueParam(value);
        } else if (key == "reverse_titles_exp") {
            Exps.ReverseTitlesExp = IsTrueParam(value);
        } else if (key == "no_open_graph_in_titles") {
            Exps.OpenGraphInTitles = !IsTrueParam(value);
        } else if (key == "title_width_reserve") {
            int width = FromStringWithDefault<int>(value);
            if (width > 0 && width < 100) {
                Exps.TitleWidthReserve = width;
            }
        } else if (key == "without_tuned_forum_classifier") {
            Exps.TunedForumClassifier = !IsTrueParam(value);
        } else if (key == "url_regions_in_titles") {
            Exps.UrlRegionsInTitles = IsTrueParam(value);
        } else if (key == "no_user_regions_in_titles") {
            Exps.UserRegionsInTitles = !IsTrueParam(value);
        } else if (key == "mn_relev_region") {
            Exps.MnRelevRegion = IsTrueParam(value);
        } else if (key == "mn_relev_region_threshold") {
            float threshold = 0.0f;
            if (TryFromString(value, threshold)) {
                Exps.MnRelevRegionThreshold = threshold;
            }
        } else if (key == "static_data_only") {
            Exps.UseStaticDataOnly = IsTrueParam(value);
        } else if (key == "two_line_titles") {
            Exps.TwoLineTitles = IsTrueParam(value);
        } else if (key == "no_meta_descriptions_in_titles") {
            Exps.MetaDescriptionsInTitles = !IsTrueParam(value);
        } else if (key == "bna_snippets") {
            Exps.BNASnippets = IsTrueParam(value);
        } else if (key == "bna_sitelink_width") {
            int width = FromStringWithDefault<int>(value);
            if (width > 0 && width < 1000) {
                Exps.BNASitelinksWidth = width;
            }
        } else if (key == "prod_lines_count") {
            Exps.ProdLinesCount = IsTrueParam(value);
        } else if (key == "news_titles_without_definitions") {
            Exps.DefinitionForNewsTitles = !IsTrueParam(value);
        } else if (key == "hostname_for_url_titles") {
            int type = FromStringWithDefault<int>(value);
            if (type >= 0 && type <= 4) {
                Exps.HostNameForUrlTitles = type;
            }
        } else if (key == "no_urlmenu_in_titles") {
            Exps.UrlmenuInTitles = !IsTrueParam(value);
        } else if (key == "popular_hosts_in_titles") {
            Exps.AddPopularHostNameToTitle = IsTrueParam(value);
        } else if (key == "popular_host_rank") {
            float popularHostRank;
            if (TryFromString(value, popularHostRank) &&
                popularHostRank >= 0.0f && popularHostRank <= 1.0f) {
                Exps.PopularHostRank = popularHostRank;
            }
        } else if (key == "title_part_delimiter_type") {
            int type;
            if (TryFromString(value, type) &&
                type >= 0 && type < DELIM_COUNT) {
                Exps.TitlePartDelimiterType = static_cast<EDelimiterType>(type);
            }
        } else if (key == "trash_pess_tr") {
            Exps.TrashPessTr = IsTrueParam(value);
        } else if (key == "no_sahibinden_titles") {
            Exps.BuildSahibindenTitles = !IsTrueParam(value);
        } else if (key == "no_caps_each_word_letter_titles") {
            Exps.CapitalizeEachWordTitleLetter = !IsTrueParam(value);
        } else if (key == "switch_off_replacer") {
            Exps.SwitchedOffReplacers.insert(value);
        } else if (key == "switch_off_replacer_extsnip") {
            Exps.SwitchedOffExtendedReplacers.insert(value);
        } else if (key == "no_forums_in_titles") {
            Exps.ForumsInTitles = !IsTrueParam(value);
        } else if (key == "no_catalogs_in_titles") {
            Exps.CatalogsInTitles = !IsTrueParam(value);
        } else if (key == "formula1940") {
            Exps.Formula1940 = IsTrueParam(value);
        } else if (key == "formula29410") {
            Exps.Formula29410 = IsTrueParam(value);
        } else if (key == "formula28883") {
            Exps.Formula28883 = IsTrueParam(value);
        } else if (key == "formula28185") {
            Exps.Formula28185 = IsTrueParam(value);
        } else if (key == "formula1938") {
            Exps.Formula1938 = IsTrueParam(value);
        } else if (key == "formula2121") {
            Exps.Formula2121 = IsTrueParam(value);
        } else if (key == "formula_s1c10k") {
            Exps.FormulaSnip1Click10k = IsTrueParam(value);
        } else if (key == "formula_s1c20k") {
            Exps.FormulaSnip1Click20k = IsTrueParam(value);
        } else if (key == "formula_s2c10k") {
            Exps.FormulaSnip2Click10k = IsTrueParam(value);
        } else if (key == "formula_s2c20k") {
            Exps.FormulaSnip2Click20k = IsTrueParam(value);
        } else if (key == "formula_s3nc10k") {
            Exps.FormulaSnip3NoClick10k = IsTrueParam(value);
        } else if (key == "formula_s3nc20k") {
            Exps.FormulaSnip3NoClick20k = IsTrueParam(value);
        }
        // break to avoid fatal error C1061: compiler limit: blocks nested too deeply
        if (key == "titles_without_splitted_hostnames") {
            Exps.TitlesWithoutSplittedHostnames = IsTrueParam(value);
        } else if (key == "without_url_based_titles") {
            Exps.GenerateUrlBasedTitles = !IsTrueParam(value);
        } else if (key == "swap_with_prod") {
            Exps.SwapWithProd = IsTrueParam(value);
        } else if (key == "no_cyr_pess") {
            Exps.CyrPessimization = !IsTrueParam(value);
        } else if (key == "ignore_dup_ext") {
            Exps.IgnoreDuplicateExtensions = IsTrueParam(value);
        } else if (key == "allow_dup_ext_tr") {
            Exps.AllowDuplicateExtensionsInTurkey = IsTrueParam(value);
        } else if (key == "ext_ratio") {
            Exps.ExtRatio = FromStringWithDefault<int>(value);
        } else if (key == "diff_ratio") {
            Exps.DiffRatio = FromStringWithDefault<int>(value);
        } else if (key == "prod_markers") {
            Exps.SnipExpProd = IsTrueParam(value);
        } else if (key == "almost_user_words") {
            Exps.AlmostUserWords = IsTrueParam(value);
        } else if (key == "syn_count") {
            Exps.SynCount = IsTrueParam(value);
        } else if (key == "inf_read_formula") {
            Exps.InfReadFormula = IsTrueParam(value);
        } else if (key == "lines_count_boost") {
            Exps.LinesCountBoost = IsTrueParam(value);
        } else if (key == "all_marks_boost") {
            Exps.UseAllMarksBoost = IsTrueParam(value);
        } else if (key == "without_def_idf") {
            Exps.DefIdf = !IsTrueParam(value);
        } else if (key == "strict_headers_only") {
            Exps.UseStrictHeadersOnly = IsTrueParam(value);
        } else if (key == "manual_with_plm_like") {
            Exps.WithPLMLikeBoostInTurkey = IsTrueParam(value);
        } else if (key == "manual_without_plm_like") {
            Exps.WithoutPLMLikeBoost = IsTrueParam(value);
        } else if (key == "informativeness"){
            Exps.UseActiveLearningInformativeness = IsTrueParam(value);
        } else if (key == "random"){
            Exps.UseRandomWithoutBoost = IsTrueParam(value);
        } else if (key == "random_with_boost"){
            Exps.UseRandomWithBoost = IsTrueParam(value);
        } else if (key == "new_marks_boost"){
            Exps.UseNewMarksBoost = IsTrueParam(value);
        } else if (key == "all_mpairwise_boost"){
            Exps.UseAllMPairwiseBoost = IsTrueParam(value);
        } else if (key == "active_learning_boost_tr"){
            Exps.UseActiveLearningBoostTR = IsTrueParam(value);
        } else if (key == "rdots_boost_lite_tr"){
            Exps.UseRdotsBoostLiteTR = IsTrueParam(value);
        } else if (key == "rdots_boost_tr"){
            Exps.UseRdotsBoostTR = IsTrueParam(value);
        } else if (key == "active_learning"){
            Exps.UseActiveLearning = IsTrueParam(value);
        } else if (key == "active_learning_boost"){
            Exps.UseActiveLearningBoost = IsTrueParam(value);
        } else if (key == "extend") {
            Exps.Extend = IsTrueParam(value);
        } else if (key == "extdiff") {
            Exps.ExtendDiff = IsTrueParam(value);
        } else if (key == "fstann" || key == "fstann2") {
            Exps.ForceStann = IsTrueParam(value);
        } else if (key == "nofstann") {
            Exps.NoStann = IsTrueParam(value);
        } else if (key == "fstannnoseg") {
            Exps.PreviewSegmentsOff = IsTrueParam(value);
        } else if (key == "fstann_schema") {
            Exps.PreviewSchema = IsTrueParam(value);
        } else if (key == "fstann_vthumb") {
            Exps.AllowVthumbPreview = IsTrueParam(value);
        } else if (key == "seaimages") {
            Exps.MaskBaseImagesPreview = value == "0";
            Exps.DropBaseImagesPreview = value == "";
        } else if (key == "fstannlen") {
            Exps.FStannLen = FromStringWithDefault<int>(value);
        } else if (key == "fstanntitlen") {
            Exps.FStannTitleLen = FromStringWithDefault<int>(value);
        } else if (key == "div2off") {
            Exps.UseDiv2 = !IsTrueParam(value);
        } else if (key == "al3topl" && !!value) {
            int len = FromStringWithDefault<int>(value);
            if (len > 0 && len < 1000) {
                Exps.Algo3TopLen = len;
            }
        } else if (key == "factors_to_dump") {
            AddFactorNums(value, Exps.FactorsToDump);
        } else if (key == "dump_weight") {
            Exps.FactorsToDump.DumpFormulaWeight = IsTrueParam(value);
        } else if (key == "finaldump") {
            Exps.FinalDump = IsTrueParam(value);
        } else if (key == "finaldumpbin") {
            Exps.FinalDumpBinary = IsTrueParam(value);
        } else if (key == "dumpall") {
            Exps.DumpAllPairs = IsTrueParam(value);
        } else if (key == "dumpm") {
            Exps.DumpManual = IsTrueParam(value);
        } else if (key == "dumpu") {
            Exps.DumpUnused = IsTrueParam(value);
        } else if (key == "dumpforpool") {
            Exps.DumpForPool = IsTrueParam(value);
        } else if (s.StartsWith("dump")) {
            Exps.DumpCands = true;
            if (s.size() == 5) {
                switch (s[4])
                {
                    case 'c':
                        Exps.DumpMode = 1;
                        break;
                    case 'p':
                        Exps.DumpMode = 2;
                        break;
                    case 'P':
                        Exps.DumpMode = 3;
                        break;
                    case 'F':
                        Exps.DumpMode = 4;
                        break;
                    default:
                        Exps.DumpMode = 0;
                };
            }
        } else if (key == "shits1") {
            Exps.Hits1 = IsTrueParam(value);
        } else if (key == "shits2") {
            Exps.Hits2 = IsTrueParam(value);
        } else if (key == "shits3") {
            Exps.Hits3 = IsTrueParam(value);
        } else if (key == "shits4") {
            Exps.Hits4 = IsTrueParam(value);
        } else if (key == "shits5") {
            Exps.Hits5 = IsTrueParam(value);
        } else if (key == "shits6") {
            Exps.Hits6 = IsTrueParam(value);
        } else if (key == "markpar") {
            Exps.MarkPar = IsTrueParam(value);
        } else if (key == "redump") {
            Exps.Redump = value;
        } else if (key.StartsWith("retext")) {
            Exps.Retexts[key].first = UTF8ToWide(value);
        } else if (key.StartsWith("titleretext")) {
            Exps.Retexts[key.substr(5)].second = UTF8ToWide(value);
        }
        // Visual C++ has the limit of 128 nested blocks, so we need a break in this else-if chain
        if (key == "foo") {
            Exps.Foo = IsTrueParam(value);
        } else if (key == "bar") {
            Exps.Bar = IsTrueParam(value);
        } else if (key == "baz") {
            Exps.Baz = IsTrueParam(value);
        } else if (key == "qux") {
            Exps.Qux = IsTrueParam(value);
        } else if (key == "fred") {
            Exps.Fred = IsTrueParam(value);
        } else if (key == "nowrep") {
            Exps.WeightedReplacers = !IsTrueParam(value);
        } else if (s.StartsWith("loss_words")) {
            Exps.LossWords = true;
            if (s.find("notitle") != TStringBuf::npos)
                Exps.LossWordsTitle = false;
            if (s.find("dump") != TStringBuf::npos)
                Exps.LossWordsDump = true;
            if (s.find("exact") != TStringBuf::npos)
                Exps.LossWordsExact = true;
        } else if (key == "nocatoptout") {
            Exps.CatalogOptOutAllowed = !IsTrueParam(value);
        } else if (key == "imgbuild") {
            Exps.IsBuildForImg = IsTrueParam(value);
        } else if (key == "videobuild") {
            Exps.IsBuildForVideo = IsTrueParam(value);
        } else if (key == "unpall") {
            Exps.UnpAll = IsTrueParam(value);
        } else if (key == "rawall") {
            Exps.RawAll = IsTrueParam(value);
        } else if (key == "allowcyrfortr") {
            Exps.AllowCyrForTr = IsTrueParam(value);
        } else if (key == "videoblock") {
            Exps.VideoBlock = IsTrueParam(value);
        } else if (key == "fvideoblock") {
            Exps.ForceVideoBlock = IsTrueParam(value);
        } else if (key == "bypassrepl") {
            Exps.BypassReplacers = IsTrueParam(value);
        } else if (s.StartsWith("uil_")) {
            Exps.ForceUILangCode = LanguageByName(s.substr(4));
        } else if (key == "yandex_width"){
            int width = FromStringWithDefault<int>(value);
            if (width > 0 && width < 1000) {
                Exps.YandexWidth = width;
            }
        } else if (key == "disableogtext") {
            Exps.DisableOgTextReplacer = IsTrueParam(value);
        } else if (key == "videoexp") {
            Exps.IsVideoExp = IsTrueParam(value);
        } else if (key == "list-off") {
            Exps.UseListSnip = !IsTrueParam(value);
        } else if (key == "list-ds") {
            Exps.DropListStat = IsTrueParam(value);
        } else if (key == "list-dh") {
            Exps.DropListHeader = IsTrueParam(value);
        } else if (key == "table") {
            Exps.UseTableSnip = IsTrueParam(value);
        } else if (key == "table-drop-tall") {
            Exps.DropTallTables = IsTrueParam(value);
        } else if (key == "table-block-broken") {
            Exps.DropBrokenTables = IsTrueParam(value);
        } else if (key == "table-ds") {
            Exps.DropTableStat = IsTrueParam(value);
        } else if (key == "table-dh") {
            Exps.DropTableHeader = IsTrueParam(value);
        } else if (key == "table-sr") {
            Exps.ShrinkTableRows = IsTrueParam(value);
        } else if (key == "nodbg") {
            Exps.NoDbg = IsTrueParam(value);
        } else if (key == "messed") { // [0..100]
            Exps.ShareMessedSnip = FromStringWithDefault<int>(value);
        } else if (key.StartsWith("shuffle")) {
            Exps.ShuffleHilite = IsTrueParam(value);
        } else if (key.StartsWith("nohilite")) {
            Exps.NoHilite = IsTrueParam(value);
        } else if (key.StartsWith("addhilite")) {
            Exps.AddHilite = IsTrueParam(value);
        } else if (key == "bring_dmoz_titles_back") {
            Exps.SuppressDmozTitles = !IsTrueParam(value);
        } else if (key == "alt_count") {
            int count;
            if (TryFromString(value, count)
                && count > 0
                && count <= MAX_ALT_SNIPPETS)
            {
                Exps.AltSnippetCount = count;
            }
        } else if (key.StartsWith("alt_")) {
            ParseAltSnippetExps(key, value);
        } else if (key == "form_raw_preview") {
            Exps.NeedFormRawPreview = IsTrueParam(value);
        } else if (key == "revrustr") {
            Exps.ReverRusTrash = IsTrueParam(value);
        } else if (key == "s2470a") {
            Exps.Snippets2470a = IsTrueParam(value);
        } else if (key == "without_mcontent_boost") {
            Exps.UseMContentBoost = !IsTrueParam(value);
        } else if (key == "use_bad_segments") {
            Exps.UseBadSegments = IsTrueParam(value);
        } else if (key == "schema_video_object") {
            Exps.UseSchemaVideoObj = IsTrueParam(value);
        } else if (key == "forum_preview") {
            Exps.ForumPreview = IsTrueParam(value);
        }
        // Visual C++ has the limit of 128 nested blocks, so we need one more break in this else-if chain
        if (key == "robots_txt_stub") {
            Exps.UseRobotsTxtStub = IsTrueParam(value);
        } else if (key == "robots_txt_skip_nps") {
            Exps.RobotsTxtSkipNPS = IsTrueParam(value);
        } else if (key == "snippets1874") {
            Exps.Snippets1874 = IsTrueParam(value);
        } else if (key == "kpbold") {
            Exps.KpBold = IsTrueParam(value);
        } else if (key == "kpreg") {
            Exps.KpReg = IsTrueParam(value);
        } else if (key == "kptit") {
            Exps.KpTit = IsTrueParam(value);
        } else if (key == "kpgg") {
            Exps.KpGg = IsTrueParam(value);
        } else if (key == "brkpct") {
            Exps.KpPct = static_cast<size_t>(100) - FromStringWithDefault<size_t>(value);
        } else if (key == "brkprob") {
            Exps.KpProb = static_cast<size_t>(100) - FromStringWithDefault<size_t>(value);
        } else if (key == "nohltit") {
            Exps.UnpaintTitle = IsTrueParam(value);
        } else if (key == "snip_len") {
            Exps.RequestedSnippetLength = FromStringWithDefault<size_t>(value);
        } else if (key == "commercial_snip_len_row") {
            Exps.CommercialSnippetLengthRow = FromStringWithDefault<float>(value);
        } else if (key == "snip_len_row") {
            Exps.RequestedSnippetLengthRow = FromStringWithDefault<float>(value);
        } else if (key == "pad_snip_len_row") {
            Exps.RequestedPadSnippetLengthRow = FromStringWithDefault<float>(value);
        } else if (key == "fml" && DynamicModels) {
            const auto formula = DynamicModels->GetMatrixnet(value);
            if (formula) {
                Exps.DynamicFormula.Reset(new TMxNetFormula(value, GetFactorDomainForSlices(*formula), *formula));
            }
        } else if (key == "default_fml" && DynamicModels) {
            const auto formula = DynamicModels->GetMatrixnet(value);
            if (formula) {
                Exps.DefaultDynamicFormula.Reset(new TMxNetFormula(value, GetFactorDomainForSlices(*formula), *formula));
            }
        } else if (key == "factsnip_fml" && DynamicModels) {
            const auto formula = DynamicModels->GetMatrixnet(value);
            if (formula) {
                Exps.DynamicFactSnippetFormula.Reset(new TMxNetFormula(value, GetFactorDomainForSlices(*formula), *formula));
            }
        } else if (key == "default_factsnip_fml" && DynamicModels) {
            const auto formula = DynamicModels->GetMatrixnet(value);
            if (formula) {
                Exps.DefaultDynamicFactSnippetFormula.Reset(new TMxNetFormula(value, GetFactorDomainForSlices(*formula), *formula));
            }
        } else if (key == "noextdw") {
            Exps.NoExtdw = IsTrueParam(value);
        } else if (key == "tophits") {
            Exps.HitsTopLength = FromStringWithDefault<ui32>(value);
        } else if (key == "bna_titlefont") {
            Exps.BNATitleFontSize = FromStringWithDefault<float>(value);
        } else if (key == "titlefont") {
            Exps.TitleFontSize = FromStringWithDefault<float>(value);
        } else if (key == "snipfont") {
            Exps.SnipFontSize = FromStringWithDefault<float>(value);
        } else if (key == "nopooraltserase") {
            Exps.ErasePoorAlts = !IsTrueParam(value);
        } else if (key == "use_touch_snippet_title"){
            Exps.UseTouchSnippetTitle = IsTrueParam(value);
        } else if (key == "use_touch_snippet_body"){
            Exps.UseTouchSnippetBody = IsTrueParam(value);
        } else if (key == "use_extsnip_row_limit"){
            Exps.UseExtSnipRowLimit = IsTrueParam(value);
        } else if (key == "extsnip_row_limit"){
            ui32 limit = 0;
            if (TryFromString(value, limit) && limit < 20)
                Exps.ExtSnipRowLimit = limit;
        } else if (key == "touch_row_scale") {
            Exps.TouchRowScale = FromStringWithDefault<float>(value, 1.0f);
        } else if (key == "pad_row_scale") {
            Exps.PadRowScale = FromStringWithDefault<float>(value, 1.0f);
        } else if (key == "title_row_limit") {
            Exps.TitleRowLimit = FromStringWithDefault<ui32>(value);
        } else if (key == "title_bna_row_limit") {
            Exps.TitleBnaRowLimit = FromStringWithDefault<float>(value);
        } else if (key == "max_bna_snip_row_count") {
            Exps.MaxBnaSnipRowCount = FromStringWithDefault<ui32>(value);
        } else if (key == "max_bna_real_snip_row_count") {
            Exps.MaxBnaRealSnipRowCount = FromStringWithDefault<ui32>(value);
        } else if (key == "touch_title_row_limit") {
            Exps.TouchTitleRowLimit = FromStringWithDefault<ui32>(value);
        } else if (key == "report") {
            Exps.Report = value;
        } else if (key == "genericmobile") {
            Exps.GenericForMobile = IsTrueParam(value);
        } else if (key == "youtube_channel_image") {
            Exps.UseYoutubeChannelImage = IsTrueParam(value);
        } else if (key == "use_pad_snippets") {
            Exps.UsePadSnippets = IsTrueParam(value);
        } else if (key == "use_mobile_url") {
            Exps.UseMobileUrl = IsTrueParam(value);
        } else if (key == "hide_urlmenu_for_mobile_url") {
            Exps.HideUrlMenuForMobileUrl = IsTrueParam(value);
        } else if (key == "links_in_snip") {
            Exps.LinksInSnip = IsTrueParam(value);
        } else if (key == "poimage") {
            Exps.UseProductOfferImage = IsTrueParam(value);
        } else if (key == "murlred") {
            Exps.MobileHilitedUrlReduction = FromStringWithDefault<ui32>(value);
        } else if (key == "symbol_cut_fragment_width") {
            Exps.SymbolCutFragmentWidth = FromStringWithDefault<float>(value, DEFAULT_SYMBOL_CUT_FRAGMENT_WIDTH);
        } else if (key == "isnip_data_keys") {
            Exps.ISnipDataKeys = value;
        } else if (key == "min_symbols_before_hyphenation") {
            Exps.MinSymbolsBeforeHyphenation = FromStringWithDefault<size_t>(value);
        } else if (key == "min_word_length_for_hyphenation") {
            Exps.MinWordLengthForHyphenation = FromStringWithDefault<size_t>(value);
        } else if (key == "top_candidate_count") {
            Exps.TopCandidateCount = FromStringWithDefault<size_t>(value);
        } else if (key == "top_candidate_skip_algo") {
            Exps.TopCandidateSkippedAlgos.insert(value);
        } else if (key == "top_candidate_factors_to_dump") {
            AddFactorNums(value, Exps.TopCandidateFactorsToDump);
        } else if (key == "top_candidate_take_algo") {
            Exps.TopCandidateTakenAlgos.insert(value);
        } else if (key == "fact_snip_top_candidate_count") {
            Exps.FactSnipTopCandidateCount = FromStringWithDefault<size_t>(value);
        } else if (key == "factor_boost") {
            TStringBuf numString;
            size_t numValue = 0;
            TStringBuf boostString;
            double boostValue = 0.;
            if (TStringBuf(value).TrySplit(':', numString, boostString) &&
                TryFromString<size_t>(numString, numValue) &&
                TryFromString<double>(boostString, boostValue))
            {
                Exps.FactorBoost.emplace_back(numValue, boostValue);
            }
        } else if (key == "factsnip_factor_boost") {
            TStringBuf numString;
            size_t numValue = 0;
            TStringBuf boostString;
            double boostValue = 0.;
            if (TStringBuf(value).TrySplit(':', numString, boostString) &&
                TryFromString<size_t>(numString, numValue) &&
                TryFromString<double>(boostString, boostValue))
            {
                Exps.FactSnippetFactorBoost.emplace_back(numValue, boostValue);
            }
        } else if (key == "len_boost") {
            TStringBuf report, boostString;
            double boostValue = 0.;
            if (TStringBuf(value).TrySplit(':', report, boostString) && TryFromString<double>(boostString, boostValue)) {
                Exps.Report2RealLenBoost.emplace(report, boostValue);
            }
        } else if (key == "candidate_degrade_percent") {
            ui32 percent = 0;
            if (TryFromString<ui32>(value, percent))
                Exps.FormulaDegradeThreshold = percent;
        } else if (key == "append_random_words") {
            ui32 limit = 0;
            if (TryFromString(value, limit))
                Exps.AppendRandomWordsCount = limit;
        } else if (key == "force_cut_snip") {
            Exps.ForceCutSnip = FromStringWithDefault<size_t>(value);
        } else if (key == "greenurltriekey") {
            Exps.GreenUrlDomainTrieKey = value;
        } else if (key == "check_passage_repeats_title") {
            Exps.CheckPassageRepeatsTitleAlgo = FromStringWithDefault<ui32>(value);
        } else if (key == "passage_repeats_title_pessimize_factor") {
            Exps.PassageRepeatsTitlePessimizeFactor = FromStringWithDefault<float>(value);
        } else if (key == "allow_breve_in_title") {
            Exps.AllowBreveInTitle = IsTrueParam(value);
        } else if (key == "bag_of_words_intersection_threshold") {
            Exps.BagOfWordsIntersectionThreshold = FromStringWithDefault<double>(value, 0.8);
        }

        return true;
    }
};

void TConfig::TImpl::AppendExps(const TStringBuf& exps) {
    TExpsConsumer c(DynamicModels, Exps);
    TCharDelimiter<const char> d(',');
    SplitString(exps.data(), exps.data() + exps.size(), d, c);
}

void TConfig::TImpl::LoadRP(const NProto::TSnipReqParams& in) {
    RP.Load(in);
    Exps.Reset();
    AppendExps(RP.SnippetExperiments);
}

void TConfig::LogPerformance(const TString& event) const
{
    if (Impl->EventLog) {
        Impl->EventLog->LogEvent(NEvClass::TSnippetsPerfomance(Impl->ReqId, Impl->DocId, event));
    }
}

const TMxNetFormula& TConfig::GetAll() const
{
    if (Impl->Exps.DynamicFormula) {
        return *Impl->Exps.DynamicFormula;
    }
    if (VideoSearch()) {
        if (UseTurkey()) {
            return AllMxNetVideoTr;
        } else {
            return AllMxNetVideo;
        }
    }
    if (Impl->Exps.Formula1940)
        return AllMxNetFormula1940;
    if (Impl->Exps.Formula29410)
        return AllMxNetFormula29410;
    if (Impl->Exps.Formula28185)
        return AllMxNetFormula28185;
    if (Impl->Exps.Formula1938)
        return AllMxNetFormula1938;
    if (Impl->Exps.InfReadFormula)
        return InformativenessWithReadability;
    if (Impl->Exps.LinesCountBoost|| Impl->Exps.UseActiveLearningInformativeness)
         return ActiveLearningInformativeness;
    if (Impl->Exps.UseRandomWithoutBoost)
         return RandomWithoutBoost;
    if (Impl->Exps.UseRandomWithBoost)
         return RandomWithBoost;
    if (Impl->Exps.UseNewMarksBoost)
         return AllMxNetPairwise;
    if (Impl->Exps.UseAllMPairwiseBoost)
         return AllMxNetPairwiseBoost;
    if (Impl->Exps.UseActiveLearning)
         return ActiveLearning;
    if (Impl->Exps.UseActiveLearningBoost)
         return ActiveLearningBoost;
    if (Impl->Exps.UseActiveLearningBoostTR)
         return ActiveLearningBoostTR;
    if (Impl->Exps.UseRdotsBoostLiteTR)
         return RdotsBoostLiteTR;
    if (Impl->Exps.UseRdotsBoostTR)
         return RdotsBoostTR;
    if (UseTurkey() && Impl->Exps.UseAllMarksBoost)
        return AllMarksBoost;
    if (Impl->Exps.Formula28883)
        return AllMxNetFormula28883;
    if (UseTurkey() || Impl->Exps.Formula2121)
        return AllMxNetFormula2121;
    if (Impl->Exps.FormulaSnip1Click10k)
        return AllMxNetFormulaSnip1Click10k;
    if (Impl->Exps.FormulaSnip1Click20k)
        return AllMxNetFormulaSnip1Click20k;
    if (Impl->Exps.FormulaSnip2Click10k)
        return AllMxNetFormulaSnip2Click10k;
    if (Impl->Exps.FormulaSnip2Click20k)
        return AllMxNetFormulaSnip2Click20k;
    if (Impl->Exps.FormulaSnip3NoClick10k)
        return AllMxNetFormulaSnip3NoClick10k;
    if (Impl->Exps.FormulaSnip3NoClick20k)
        return AllMxNetFormulaSnip3NoClick20k;

    if (Impl->Exps.DefaultDynamicFormula) {
        return *Impl->Exps.DefaultDynamicFormula;
    }
    return AllMxNetPairwiseBoost;
}

const TLinearFormula& TConfig::GetManual() const
{
    if (Impl->Exps.DynamicManualBoost)
        return *Impl->Exps.DynamicManualBoost;
    if (VideoSearch()) {
        return ManualVideoFormula;
    }
    if (Impl->Exps.Formula1940)
        return ManualFormula1940;
    if (Impl->Exps.Formula29410)
        return ManualFormula29410;
    if (Impl->Exps.InfReadFormula)
        return ManualFormulaInfRead;
    if (Impl->Exps.LinesCountBoost)
        return ManualFormulaWithLinesCount;
    if (CalculatePLM())
        return ManualFormulaWithPLMLike;
    return ManualFormula;
}

TFormula TConfig::GetTotalFormula() const
{
    const TFactorDomain* factorsDomain = &GetAll().GetFactorDomain();
    // allow to redefine factors domain
    // only if the selected formula does not use extended factors
    if (*factorsDomain == algo2Domain) {
        if (ExpFlagOn("use_web_factors"))
            factorsDomain = &algo2PlusWebDomain;
        if (ExpFlagOn("use_web_noclick_factors"))
            factorsDomain = &algo2PlusWebNoClickDomain;
        if (ExpFlagOn("use_web_v1_factors"))
            factorsDomain = &algo2PlusWebV1Domain;
    }
    return TFormula(GetAll(), GetManual(), *factorsDomain);
}

const TLinearFormula& TConfig::GetFactSnippetManual() const
{
    if (Impl->Exps.FactSnippetDynamicManualBoost) {
        return *Impl->Exps.FactSnippetDynamicManualBoost;
    }
    return FactSnippetManualFormula;
}

const TMxNetFormula& TConfig::GetFactSnippetFormula() const {
    if (Impl->Exps.DynamicFactSnippetFormula) {
        return *Impl->Exps.DynamicFactSnippetFormula;
    }
    if (Impl->Exps.DefaultDynamicFactSnippetFormula) {
        return *Impl->Exps.DefaultDynamicFactSnippetFormula;
    }
    return AllMxNetPairwiseBoost;
}

TFormula TConfig::GetFactSnippetTotalFormula() const
{
    const TFactorDomain* factorsDomain = &GetFactSnippetFormula().GetFactorDomain();
    // allow to redefine factors domain
    // only if the selected formula does not use extended factors
    if (*factorsDomain == algo2Domain) {
        if (ExpFlagOn("use_web_factors_in_factsnip"))
            factorsDomain = &algo2PlusWebDomain;
        if (ExpFlagOn("use_web_noclick_factors_in_factsnip"))
            factorsDomain = &algo2PlusWebNoClickDomain;
        if (ExpFlagOn("use_web_v1_factors_in_factsnip"))
            factorsDomain = &algo2PlusWebV1Domain;
    }
    return TFormula(GetFactSnippetFormula(), GetFactSnippetManual(), *factorsDomain);
}

bool TConfig::ImgSearch() const {
    return Impl->Exps.IsBuildForImg || Impl->RP.IsBuildForImg;
}

const NSc::TValue& TConfig::GetEntityData() const {
    return Impl->RP.EntityData;
}

bool TConfig::IsPolitician() const {
    return Impl->RP.IsPolitician;
}

TLangMask TConfig::GetQueryLangMask() const {
    return Impl->RP.QueryLangMask;
}

ui64 TConfig::GetRelevRegion() const {
    return Impl->RP.RelevRegion;
}

//video stuff start
bool TConfig::VideoSearch() const {
    return Impl->Exps.IsBuildForVideo || Impl->RP.IsBuildForVideo;
}
bool TConfig::VideoTitles() const {
    return VideoSearch();
}

bool TConfig::VideoSnipAlgo() const {
    return VideoSearch() && GetMaxSnipCount() > 0;
}

bool TConfig::IsVideoExp() const {
    return Impl->Exps.IsVideoExp;
}

bool TConfig::VideoLength() const {
    return VideoSearch();
}

bool TConfig::RequireSentAttrs() const {
    return VideoSearch();
}

bool TConfig::VideoHide() const {
    return VideoSearch();
}

//video stuff end

//length stuff start
int TConfig::GetMaxSnipCount() const {
    if (Impl->Exps.MaxSnipCount > 0) {
        return Impl->Exps.MaxSnipCount;
    }
    if (!Impl->RP.AbsentT) {
        return Impl->RP.MaxSnippetCount;
    } else {
        if (VideoSearch()) {
            return 1;
        }
        if (GetBigDescrForMainBna() && GetMainBna()) {
            return 1;
        }
        return MAX_SNIP_COUNT;
    }
}

int TConfig::GetMaxSnipRowCount() const {
    if (Impl->Exps.MaxSnipCount > 0) {
        return Impl->Exps.MaxSnipCount;
    }
    if (!Impl->RP.AbsentT) {
        return Impl->RP.MaxSnippetCount;
    }
    return 2;
}

int TConfig::GetMaxTitleLen() const {
    return Impl->RP.MaxTitleLength;
}

int TConfig::GetMaxDefinitionLen() const {
    return Impl->RP.MaxTitleStartLen;
}

int TConfig::GetTitlePixelsInLine() const {
    if (BNASnippets()) {
        return Impl->Exps.BNASitelinksWidth;
    }
    if (Impl->RP.MaxTitlePixelLen) {
        return Impl->RP.MaxTitlePixelLen;
    }
    if (UseTouchSnippetTitle()) {
        const int snipWidth = Impl->RP.SnipWidthWasSet ? GetSnipWidth() : 290;
        return snipWidth - Impl->Exps.TitleWidthReserve;
    }
    int snipWidth = (IsTouchReport() ? DEFAULT_SNIP_WIDTH : GetSnipWidth());
    return snipWidth - Impl->Exps.TitleWidthReserve;
}

float TConfig::GetMaxTitleLengthInRows() const {
    if (GetMainBna() && IsTouchReport() && Impl->Exps.TitleBnaRowLimit > 0.5f) {
        return Impl->Exps.TitleBnaRowLimit;
    }
    if (Impl->Exps.TitleRowLimit) {
        return static_cast<float>(Impl->Exps.TitleRowLimit);
    }
    if (UseTouchSnippetTitle()) {
        return static_cast<float>(Impl->Exps.TouchTitleRowLimit);
    }
    if (IsPadReport() && !IsCommercialQuery()) {
        return 2.0f;
    }
    return 1.0f;
}

float TConfig::GetTitleFontSize() const {
    if (BNASnippets()) {
        return 16.0f;
    }
    if (GetMainBna()) {
        return Impl->Exps.BNATitleFontSize;
    }
    if (Impl->Exps.TitleFontSize) {
        return Impl->Exps.TitleFontSize;
    }
    if (Impl->Exps.UsePadSnippets && IsPadReport()) {
        return 20.0f;
    }
    if (UseTouchSnippetTitle()) {
        return 20.0f;
    }
    return 18.0f;
}

float TConfig::GetSnipFontSize() const {
    if (Impl->Exps.SnipFontSize) {
        return Impl->Exps.SnipFontSize;
    }
    if (Impl->Exps.UsePadSnippets && IsPadReport()) {
        return 16.0f;
    }
    if (UseTouchSnippetBody()) {
        return 15.0f;
    }
    return 13.0f;
}

ui32 TConfig::GetMaxUrlLength() const {
    return Impl->RP.MaxUrlLength;
}

ui32 TConfig::GetUrlCutThreshold() const {
    return Impl->RP.UrlCutThreshold;
}

ui32 TConfig::GetMobileHilitedUrlReduction() const {
    return Impl->Exps.MobileHilitedUrlReduction;
}

ui32 TConfig::GetMaxUrlmenuLength() const {
    return Impl->RP.MaxUrlmenuLength;
}

ui64 TConfig::GetUserReqHash() const {
    return Impl->RP.UserReqHash;
}

ETextCutMethod TConfig::GetTitleCutMethod() const {
    return Impl->RP.TitleCutMethod;
}

// SNIPPETS-1139
ETextCutMethod TConfig::GetSnipCutMethod() const {
    if (ImgSearch() || VideoSearch()) {
        return TCM_SYMBOL;
    }
    return Impl->RP.SnippetCutMethod;
}

float TConfig::GetRequestedMaxSnipLen() const {
    if (GetSnipCutMethod() == TCM_SYMBOL) {
        const int requestedLen = Impl->Exps.RequestedSnippetLength ? Impl->Exps.RequestedSnippetLength :
                Impl->RP.RequestedSnippetLength;
        if (requestedLen) {
            return requestedLen;
        }
    } else {
        float requestedLenRow = (IsPadReport() && !IsCommercialQuery()) ?
                                Impl->Exps.RequestedPadSnippetLengthRow :
                                Impl->Exps.RequestedSnippetLengthRow;
        if (IsCommercialQuery() && Impl->Exps.CommercialSnippetLengthRow > MINIMAL_SNIPPET_ROW_LEN) {
            requestedLenRow = Impl->Exps.CommercialSnippetLengthRow;
        }
        if (Impl->RP.RequestedSnippetLengthRow > 0) {
            requestedLenRow = Impl->RP.RequestedSnippetLengthRow;
        }
        if (requestedLenRow > MINIMAL_SNIPPET_ROW_LEN) {
            return requestedLenRow;
        }
    }
    return 0;
}

int TConfig::GetMaxByLinkSnipCount() const {
    return 1;
}
//length stuff end

//dump stuff start
bool TConfig::IsDumpCandidates() const {
    return Impl->RP.DumpCandidates || Impl->Exps.DumpCands;
}
bool TConfig::IsDumpForPoolGeneration() const {
    return Impl->RP.DumpForPoolGeneration || Impl->Exps.DumpForPool;
}
bool TConfig::IsDumpAllAlgo3Pairs() const {
    return IsDumpCandidates() && (Impl->RP.DumpAllAlgo3Pairs || Impl->Exps.DumpAllPairs);
}
bool TConfig::IsDumpWithUnusedFactors() const {
    return GetInfoRequestType() == INFO_SNIPPETS || (IsDumpCandidates() && (Impl->RP.DumpWithUnusedFactors || Impl->Exps.DumpUnused));
}
bool TConfig::IsDumpWithManualFactors() const {
    return GetInfoRequestType() == INFO_SNIPPETS || (IsDumpCandidates() && (Impl->RP.DumpWithManualFactors || Impl->Exps.DumpManual));
}
int TConfig::DumpCoding() const {
    return Impl->Exps.DumpMode;
}

double TConfig::BagOfWordsIntersectionThreshold() const {
    return Impl->Exps.BagOfWordsIntersectionThreshold;
}

// SNIPPETS-3559
TFactorsToDump TConfig::FactorsToDump() const {
    return Impl->Exps.FactorsToDump;
}
bool TConfig::IsDumpFinalFactors() const {
    return Impl->Exps.FinalDump;
}
bool TConfig::IsDumpFinalFactorsBinary() const {
    return Impl->Exps.FinalDumpBinary;
}
//dump stuff end

//docsig stuff start
int TConfig::GetSignatureWidth() const {
    return Impl->RP.SignatureWidth;
}

int TConfig::GetSignatureMinLength() const {
    return Impl->RP.SignatureMinLength;
}

bool TConfig::ShouldCalculateSignature() const {
    return Impl->RP.ShouldCalculateSignature;
}

bool TConfig::ShouldReturnDocStaticSig() const {
    return Impl->RP.ShouldReturnDocStaticSig;
}

i64 TConfig::DocStaticSig() const {
    return Impl->RP.DocStaticSig;
}
//docsig stuff end

//random stuff start

bool TConfig::SkipWordstatDebug() const {
    return Impl->Exps.NoDbg;
}

bool TConfig::UseTurkey() const {
    return Impl->RP.IsTurkey && GetUILang() == LANG_TUR;
}

ui32 TConfig::GetHitsTopLength() const
{
    if (Impl->Exps.HitsTopLength) {
        return Impl->Exps.HitsTopLength;
    }
    return Impl->RP.HitsTopLength;
}

const TString& TConfig::GetRedump() const {
    return Impl->Exps.Redump;
}

const TRetexts& TConfig::GetRetexts() const {
    return Impl->Exps.Retexts;
}

bool TConfig::ErasePoorAlts() const {
    return Impl->Exps.ErasePoorAlts;
}

bool TConfig::UnpAll() const {
    return Impl->Exps.UnpAll;
}

bool TConfig::HasEntityClassRequest() const {
    return !Impl->RP.EntityData.IsNull();
}

bool TConfig::RawAll() const {
    return Impl->Exps.RawAll;
}

bool TConfig::NeedFormRawPreview() const {
    return Impl->Exps.NeedFormRawPreview;
}

bool TConfig::VideoAttrWeight() const {
    return VideoSearch();
}

//Issue: SNIPPETS-1179
bool TConfig::TitWeight() const {
    return true;
}

bool TConfig::IsMobileSearch() const {
    return Impl->RP.IsMobileSearch;
}

bool TConfig::GetRawPassages() const {
    return Impl->RP.RawPassages;
}

bool TConfig::IsYandexCom() const {
    return Impl->RP.FaceType == ftYandexCom;
}

bool TConfig::VideoLangHackTr() const {
    return Impl->RP.FaceType == ftYandexComTr && VideoSearch();
}

EFaceType TConfig::GetFaceType() const {
    return Impl->RP.FaceType;
}

const TSnipInfoReqParams& TConfig::GetInfoRequestParams() const {
    if (Impl->Exps.Hits1) {
        return Default<TInfoReqParamsDbgSet>().InfoReqParamsDbg1;
    } else  if (Impl->Exps.Hits2) {
        return Default<TInfoReqParamsDbgSet>().InfoReqParamsDbg2;
    } else if (Impl->Exps.Hits3) {
        return Default<TInfoReqParamsDbgSet>().InfoReqParamsDbg3;
    } else if (Impl->Exps.Hits4) {
        return Default<TInfoReqParamsDbgSet>().InfoReqParamsDbg4;
    } else if (Impl->Exps.Hits5) {
        return Default<TInfoReqParamsDbgSet>().InfoReqParamsDbg5;
    } else if (Impl->Exps.Hits6) {
        return Default<TInfoReqParamsDbgSet>().InfoReqParamsDbg6;
    }
    return Impl->RP.InfoReqParams;
}

bool TConfig::IsNeedExt() const {
    return Impl->RP.ExtendSnippets != 0 || Impl->Exps.Extend;
}

//Issue: SNIPPETS-1021
bool TConfig::NeedExtDiff() const {
    return Impl->RP.ExtendSnippets == 2 || Impl->Exps.ExtendDiff;
}

bool TConfig::SkipExtending() const {
    if (ImgSearch() || VideoSearch()) {
        return true;
    }
    return false;
}

//Issue: SNIPPETS-1022
bool TConfig::NeedExtStat() const {
    return !Impl->Exps.NoExtdw;
}

bool TConfig::ForceStann() const {
    return Impl->Exps.ForceStann || Impl->RP.ForceStann;
}
bool TConfig::NoPreview() const {
    return Impl->Exps.NoStann;
}
bool TConfig::MaskBaseImagesPreview() const {
    return Impl->Exps.MaskBaseImagesPreview;
}
bool TConfig::DropBaseImagesPreview() const {
    return Impl->Exps.DropBaseImagesPreview;
}
bool TConfig::SchemaPreview() const {
    return Impl->Exps.PreviewSchema;
}
bool TConfig::SchemaPreviewSegmentsOff() const {
    return Impl->Exps.PreviewSegmentsOff;
}
bool TConfig::PreviewBaseImagesHack() const {
    return false;
}
bool TConfig::PreviewBaseImagesHackV2() const {
    return Impl->Exps.Snippets2470a;
}
bool TConfig::DisallowVthumbPreview() const {
    return !Impl->Exps.AllowVthumbPreview;
}
int TConfig::FStannLen() const {
    return Impl->Exps.FStannLen ? Impl->Exps.FStannLen : Impl->RP.FStannLen ? Impl->RP.FStannLen : 500;
}

int TConfig::FStannTitleLen() const {
    return Impl->Exps.FStannTitleLen ? Impl->Exps.FStannTitleLen : Impl->RP.FStannTitleLen ? Impl->RP.FStannTitleLen : 285;
}

bool TConfig::IsAllPassageHits() const {
    return Impl->RP.AllPassageHits;
}

bool TConfig::IsPaintedRawPassages() const {
    return Impl->RP.PaintedRawPassages;
}

bool TConfig::IsMarkParagraphs() const {
    return Impl->RP.MarkParagraphs || Impl->Exps.MarkPar;
}

bool TConfig::IsSlovariSearch() const {
    return Impl->RP.SlovariSearch;
}

bool TConfig::DmozSnippetReplacerAllowed() const {
    // Impl->RP.AllowYacaSnippets is a legacy name, not a typo
    return Impl->RP.AllowYacaSnippets;
}

bool TConfig::YacaSnippetReplacerAllowed() const {
    return Impl->RP.AllowYacaSnippets;
}

bool TConfig::IsMainPage() const {
    return Impl->RP.MainPage;
}

//Issue: SNIPPETS-1350
bool TConfig::IsUseStaticDataOnly() const {
    return Impl->RP.UseStaticDataOnly || Impl->Exps.UseStaticDataOnly || Impl->Exps.BNASnippets;
}
bool TConfig::IsMainContentStaticDataSource() const {
    return Impl->RP.StaticDataSource == "maincontent";
}

bool TConfig::PaintNoAttrs() const {
    return Impl->RP.PaintNoAttrs;
}

//Issue: SNIPPETS-1023
//! Allow snippets candidates boosting with AZ_TELEPHONE zone
bool TConfig::BoostTelephoneCandidates() const {
    return true && Impl->RP.IsTelOrg;
}

bool TConfig::PaintAllPhones() const {
    return Impl->RP.IsTelOrg && false;
}

//Issue: SNIPPETS-1324
bool TConfig::IsUrlOrEmailQuery() const {
    return Impl->RP.IsUrlOrEmailQuery;
}

//Issue: SNIPPETS-1303
bool TConfig::IsNeedGenerateAltHeaderOnly() const {
    return Impl->RP.SnippetsAltHeader == 1;
}

bool TConfig::IsNeedGenerateTitleOnly() const {
    return Impl->RP.SnippetsAltHeader == 0;
}

//Issue: SAAS-2143
bool TConfig::ForcePureTitle() const {
    return Impl->RP.ForcePureTitle;
}

ELanguage TConfig::GetForeignNavHackLang() const {
    if (Impl->Exps.ForceUILangCode != LANG_UNK)
        return Impl->Exps.ForceUILangCode;
    return Impl->RP.ForeignNavHackLang;
}

// SNIPPETS-1078
bool TConfig::WeightedReplacers() const {
    return Impl->Exps.WeightedReplacers && GetUILang() == LANG_RUS;
}

const TString& TConfig::GetTLD() const {
    return Impl->RP.TLD;
}

ELanguage TConfig::GetUILang() const {
    if (Impl->Exps.ForceUILangCode != LANG_UNK)
        return Impl->Exps.ForceUILangCode;
    return Impl->RP.UILangCode;
}

// Issue: SNIPPETS-1431
bool TConfig::LinksInSnip() const {
    return Impl->Exps.LinksInSnip;
}

bool TConfig::VideoWatch() const {
    return Impl->RP.VideoWatch;
}

// SNIPPETS-1555
bool TConfig::SuppressCyrForTr() const {
    //note: video actually wants it for SNIPPETS-1850
    return GetUILang() == LANG_TUR
           && !Impl->Exps.AllowCyrForTr;
}

// SNIPPETS-2289
bool TConfig::PessimizeCyrForTr() const {
    return Impl->Exps.CyrPessimization && SuppressCyrForTr();
}

bool TConfig::UnpaintTitle() const {
    return Impl->Exps.UnpaintTitle;
}

// Issue: SNIPPETS-1011
bool TConfig::UseDiversity2() const {
    return true && Impl->Exps.UseDiv2;
}

size_t TConfig::Algo3TopLen() const {
    return Impl->Exps.Algo3TopLen;
}

// Issue: SNIPPETS-1541
bool TConfig::VideoBlock() const {
    return VideoWatch() && Impl->Exps.VideoBlock || Impl->Exps.ForceVideoBlock;
}

bool TConfig::ForceVideoTitle() const {
    return Impl->Exps.ForceVideoBlock;
}

bool TConfig::UseForumReplacer() const {
    if (IsTouchReport() || IsMobileSearch()) {
        return false;
    }
    if (GetMainBna()) { // don't build it for BNA main result - SNIPPETS-2857
        return false;
    }
    return !SwitchOffReplacer("forum");
}

bool TConfig::CatalogOptOutAllowed() const {
    return Impl->Exps.CatalogOptOutAllowed;
}

// SNIPPETS-1659
bool TConfig::BypassReplacers() const {
    return Impl->Exps.BypassReplacers;
}

// SNIPPETS-1657
bool TConfig::EnableRemoveDuplicates() const {
    return !ImgSearch() && !VideoSearch();
}

// SNIPPETS-1531
bool TConfig::UseListSnip() const {
    return Impl->Exps.UseListSnip && !IsMobileSearch() && !IsTouchReport();
}

bool TConfig::DropListStat() const {
    return Impl->Exps.DropListStat;
}

bool TConfig::DropListHeader() const {
    return Impl->Exps.DropListHeader;
}

// SNIPPETS-1452
bool TConfig::UseTableSnip() const {
    return Impl->Exps.UseTableSnip && !IsMobileSearch();
}
bool TConfig::DropTallTables() const {
    return Impl->Exps.DropTallTables;
}
bool TConfig::DropBrokenTables() const {
    return Impl->Exps.DropBrokenTables;
}
bool TConfig::DropTableStat() const {
    return Impl->Exps.DropTableStat;
}
bool TConfig::DropTableHeader() const {
    return Impl->Exps.DropTableHeader;
}
bool TConfig::ShrinkTableRows() const {
    return Impl->Exps.ShrinkTableRows;
}
// end SNIPPETS-1452

// BLNDR-5358
TString TConfig::GreenUrlDomainTrieKey() const {
    return Impl->Exps.GreenUrlDomainTrieKey;
}

// BLNDR-6145
ui32 TConfig::CheckPassageRepeatsTitleAlgo() const {
    return Impl->Exps.CheckPassageRepeatsTitleAlgo;
}

float TConfig::GetPassageRepeatsTitlePessimizeFactor() const {
    return Impl->Exps.PassageRepeatsTitlePessimizeFactor;
}

bool TConfig::UnpackFirst10() const {
    return Impl->RP.UnpackFirst10;
}

int TConfig::GetStaticAnnotationMode() const
{
    int res = SAM_HIDE_EMPTY | SAM_EXTENDED_BY_LINK | SAM_CAN_USE_CONTENT |
        SAM_CAN_USE_REFERAT;
    if (UnpackFirst10())
        res |= SAM_DOC_START;
    return res;
}

bool TConfig::ShareMessedSnip() const {
    return Impl->Exps.ShareMessedSnip;
}

bool TConfig::WantToMess(size_t value) const {
    return int(value % 100) < Impl->Exps.ShareMessedSnip;
}

bool TConfig::ShuffleHilite() const {
    return Impl->Exps.ShuffleHilite;
}

bool TConfig::NoHilite() const {
    return Impl->Exps.NoHilite;
}

bool TConfig::AddHilite() const {
    return Impl->Exps.AddHilite;
}

bool TConfig::UseBadSegments() const {
    return Impl->Exps.UseBadSegments;
}

bool TConfig::IgnoreRegionPhrase() const {
    return Impl->RP.IgnoreRegionPhrase;
}

int TConfig::GetYandexWidth() const {
    if (BNASnippets()) {
        return Impl->Exps.BNASitelinksWidth;
    }
    if (Impl->Exps.YandexWidth)
        return Impl->Exps.YandexWidth;
    const int snipWidthReserve = 8;
    if (UseTouchSnippetBody()) {
        const int snipWidth = Impl->RP.SnipWidthWasSet ? GetSnipWidth() : 290;
        return snipWidth - snipWidthReserve;
    }
    int snipWidth = (IsTouchReport() ? DEFAULT_SNIP_WIDTH : GetSnipWidth());
    return snipWidth - snipWidthReserve;
}

// SNIPPETS-4561
const TString& TConfig::GetISnipDataKeys() const {
    return Impl->Exps.ISnipDataKeys;
}

//random stuff end

// SNIPPETS-5377 start
size_t TConfig::GetMinSymbolsBeforeHyphenation() const {
    return Impl->Exps.MinSymbolsBeforeHyphenation;
}

size_t TConfig::GetMinWordLengthForHyphenation() const {
    return Impl->Exps.MinWordLengthForHyphenation;
}
// SNIPPETS-5377 end


// FACTS-504
float TConfig::GetSymbolCutFragmentWidth() const {
    return Impl->Exps.SymbolCutFragmentWidth;
}

// SNIPPETS-1864
bool TConfig::DisableOgTextReplacer() const {
    return Impl->Exps.DisableOgTextReplacer;
}

//foobar stuff start
bool TConfig::Foo() const {
    return Impl->Exps.Foo;
}
bool TConfig::Bar() const {
    return Impl->Exps.Bar;
}
bool TConfig::Baz() const {
    return Impl->Exps.Baz;
}
bool TConfig::Qux() const {
    return Impl->Exps.Qux;
}
bool TConfig::Fred() const {
    return Impl->Exps.Fred;
}
//foobar stuff end

//losswords stuff start
bool TConfig::LossWords() const {
    return Impl->Exps.LossWords;
}
bool TConfig::LossWordsTitle() const {
    return Impl->Exps.LossWordsTitle;
}
bool TConfig::LossWordsDump() const {
    return Impl->Exps.LossWordsDump;
}
bool TConfig::LossWordsExact() const {
    return Impl->Exps.LossWordsExact;
}
//losswords stuff end

bool TConfig::CalculatePLM() const {
    if (UseTurkey()) {
        return Impl->Exps.WithPLMLikeBoostInTurkey;
    }
    return !Impl->Exps.WithoutPLMLikeBoost;
}

// SNIPPETS-2061
bool TConfig::UseStrictHeadersOnly() const {
    if (Impl->Exps.ReverseTitlesExp) {
        return false;
    }
    return Impl->Exps.UseStrictHeadersOnly;
}

// SNIPPETS-2117
bool TConfig::DefIdf() const {
    if (Impl->Exps.ReverseTitlesExp) {
        return false;
    }
    return Impl->Exps.DefIdf;
}

// SNIPPETS-2119
bool TConfig::SynCount() const {
    return Impl->Exps.SynCount;
}

// SNIPPETS-2199
bool TConfig::AlmostUserWordsInTitles() const {
    if (Impl->Exps.ReverseTitlesExp) {
        return false;
    }
    return Impl->Exps.AlmostUserWords;
}

bool TConfig::UseAlmostUserWords() const {
    return Impl->Exps.AlmostUserWords || Impl->Exps.ExpsForMarkers.find("almost_user_words") != TString::npos;
}

// SnipExp markers stuff start

TString TConfig::GetExpsForMarkers() const {
    return Impl->Exps.ExpsForMarkers;
}

bool TConfig::UseSnipExpMarkers() const {
    return !Impl->Exps.ExpsForMarkers.empty();
}

bool TConfig::SnipExpProd() const {
    return Impl->Exps.SnipExpProd;
}

int TConfig::GetDiffRatio() const {
    return Impl->Exps.DiffRatio;
}

bool TConfig::SwapWithProd() const {
    return Impl->Exps.SwapWithProd;
}

bool TConfig::ProdLinesCount() const {
    return Impl->Exps.ProdLinesCount;
}

bool TConfig::TitlesDiffOnly() const {
    return Impl->Exps.TitlesDiffOnly;
}

// SnipExp markers stuff end

// SNIPPETS-2269
bool TConfig::SuppressDmozTitles() const {
    if (Impl->Exps.ReverseTitlesExp) {
        return false;
    }
    return Impl->Exps.SuppressDmozTitles && UseTurkey();
}

// SNIPPETS-2212
int TConfig::GetExtRatio() const {
    return Impl->Exps.ExtRatio;
}

bool TConfig::IgnoreDuplicateExtensions() const {
    if (Impl->Exps.IgnoreDuplicateExtensions) {
        return true;
    }
    if (UseTurkey()) {
        return !Impl->Exps.AllowDuplicateExtensionsInTurkey;
    }
    return false;
}

// SNIPPETS-2332
bool TConfig::GenerateUrlBasedTitles() const {
    if (Impl->Exps.ReverseTitlesExp) {
        return false;
    }
    return Impl->Exps.GenerateUrlBasedTitles && UseTurkey() && !VideoSearch();
}

// SNIPPETS-2210
int TConfig::AltSnippetCount() const {
    return Impl->Exps.AltSnippetCount;
}

TString TConfig::AltSnippetExps(int index) const {
    if (!AltSnippetCount() || index < 0 || index > AltSnippetCount()) {
        Y_ASSERT(false);
        return TString();
    }
    return Impl->Exps.AltSnippetExps.at(index);
}

// SNIPPETS-2375
bool TConfig::TitlesWithSplittedHostnames() const {
    if (Impl->Exps.ReverseTitlesExp) {
        return false;
    }
    return UseTurkey() && !Impl->Exps.TitlesWithoutSplittedHostnames;
}

// SNIPPETS-2442
bool TConfig::UseMContentBoost() const {
    return Impl->Exps.UseMContentBoost;
}

// SNIPPETS-2502
bool TConfig::ForumsInTitles() const {
    if (Impl->Exps.ReverseTitlesExp) {
        return false;
    }
    return Impl->Exps.ForumsInTitles;
}

// SNIPPETS-2599
bool TConfig::CatalogsInTitles() const {
    if (Impl->Exps.ReverseTitlesExp) {
        return false;
    }
    return Impl->Exps.CatalogsInTitles;
}

// SNIPPETS-2505
bool TConfig::SwitchOffReplacer(const TString& replacerName) const {
    return Impl->Exps.SwitchedOffReplacers.contains(replacerName);
}

bool TConfig::SwitchOffExtendedReplacerAnswer(const TString& replacerName) const {
    return Impl->Exps.SwitchedOffExtendedReplacers.contains(replacerName);
}

// SNIPPETS-2239
bool TConfig::BuildSahibindenTitles() const {
    if (Impl->Exps.ReverseTitlesExp) {
        return false;
    }
    return Impl->Exps.BuildSahibindenTitles;
}

bool TConfig::ForumForPreview() const {
    return Impl->Exps.ForumPreview;
}

// SNIPPETS-2444
bool TConfig::TrashPessTr() const {
    return Impl->Exps.TrashPessTr;
}

// SNIPPETS-2533
bool TConfig::AddPopularHostNameToTitle() const {
    if (Impl->Exps.ReverseTitlesExp) {
        return false;
    }
    return Impl->Exps.AddPopularHostNameToTitle;
}

float TConfig::GetPopularHostRank() const {
    return Impl->Exps.PopularHostRank;
}

EDelimiterType TConfig::GetTitlePartDelimiterType() const {
    return Impl->Exps.TitlePartDelimiterType;
}

bool TConfig::GetRankingFactor(const size_t& factorId, float& value) const {
    return Impl->RP.GetRankingFactor(factorId, value);
}

const float* TConfig::GetRankingFactors() const {
    return Impl->RP.GetRankingFactors();
}

size_t TConfig::GetRankingFactorsCount() const {
    return Impl->RP.GetRankingFactorsCount();
}

// SNIPPETS-2494
bool TConfig::UrlmenuInTitles() const {
    if (Impl->Exps.ReverseTitlesExp) {
        return false;
    }
    return Impl->Exps.UrlmenuInTitles;
}

// SNIPPETS-2524
bool TConfig::UseSchemaVideoObj() const {
    return Impl->Exps.UseSchemaVideoObj;
}

// SNIPPETS-2567
int TConfig::HostNameForUrlTitles() const {
    if (Impl->Exps.ReverseTitlesExp) {
        return 0;
    }
    return Impl->Exps.HostNameForUrlTitles;
}

// SNIPPETS-2074
bool TConfig::DefinitionForNewsTitles() const {
    if (Impl->Exps.ReverseTitlesExp) {
        return false;
    }
    return Impl->Exps.DefinitionForNewsTitles;
}

// title generating stuff start

ETitleGeneratingAlgo TConfig::GetTitleGeneratingAlgo() const {
    if (Impl->Exps.ReverseTitlesExp) {
        return TGA_SMART;
    }
    return Impl->Exps.TitleGeneratingAlgo;
}

ETitleDefinitionMode TConfig::GetDefinitionMode() const {
    if (BNASnippets()) {
        return TDM_ELIMINATE;
    }
    if (VideoTitles()) {
        return TDM_IGNORE;
    }
    if (EliminateDefinitions()) {
        return TDM_ELIMINATE;
    }
    return Impl->Exps.DefinitionMode;
}

// SNIPPETS-7265
bool TConfig::EliminateDefinitions() const {
    // in this mode we look for shorter titles without definition (usually sitename) wherever we can and disable additions
    // affects creative_work, header-based, opengraph etc, changing many different aspects
    return Impl->Exps.EliminateDefinitions;
}

bool TConfig::IsTitleContractionOk(size_t originalTitleLen, size_t newTitleLen) const {
    return newTitleLen > Impl->Exps.TitleContractionSymbolThreshold &&
           newTitleLen > Impl->Exps.TitleContractionRatioThreshold * originalTitleLen;
}

// SNIPPETS-3478
float TConfig::DefinitionPopularityThreshold() const {
    if (!IsWebReport() || Impl->Exps.DefinitionPopularityThreshold > 0.f) {
        return Impl->Exps.DefinitionPopularityThreshold;
    }
    return 0.8f;
}

// SNIPPETS-2448
bool TConfig::TitlesReadabilityExp() const {
    if (Impl->Exps.ReverseTitlesExp) {
        return false;
    }
    return Impl->Exps.TitlesReadabilityExp;
}

// title generating stuff end

// SNIPPETS-2609
bool TConfig::BNASnippets() const {
    return Impl->Exps.BNASnippets;
}

// SNIPPETS-2619
bool TConfig::GetMainBna() const {
    return Impl->RP.MainBna || Impl->Exps.MainBna;
}

// SNIPPETS-2619
bool TConfig::GetBigDescrForMainBna() const {
    return Impl->Exps.BigDescrForMainBna;
}

// SNIPPETS-5993
ui32 TConfig::GetMaxBnaSnipRowCount() const {
    return Impl->Exps.MaxBnaSnipRowCount;
}

ui32 TConfig::GetMaxBnaRealSnipRowCount() const {
    return Impl->Exps.MaxBnaRealSnipRowCount;
}

// SNIPPETS-2535
bool TConfig::UseRobotsTxtStub() const {
    return Impl->Exps.UseRobotsTxtStub;
}

// SNIPPETS-2535
bool TConfig::RobotsTxtSkipNPS() const {
    return Impl->Exps.RobotsTxtSkipNPS;
}

bool TConfig::RTYSnippets() const {
    return Impl->RP.RTYSnippets;
}

// SNIPPETS-2610
bool TConfig::MetaDescriptionsInTitles() const {
    if (Impl->Exps.ReverseTitlesExp) {
        return false;
    }
    return Impl->Exps.MetaDescriptionsInTitles;
}

// SNIPPETS-1751
bool TConfig::CapitalizeEachWordTitleLetter() const {
    if (Impl->Exps.ReverseTitlesExp) {
        return false;
    }
    return Impl->Exps.CapitalizeEachWordTitleLetter;
}

// SNIPPETS-2667
bool TConfig::TwoLineTitles() const {
    if (Impl->Exps.ReverseTitlesExp) {
        return false;
    }
    return Impl->Exps.TwoLineTitles;
}

// SNIPPETS-2693
bool TConfig::UrlRegionsInTitles() const {
    if (Impl->Exps.ReverseTitlesExp) {
        return false;
    }
    return Impl->Exps.UrlRegionsInTitles;
}

bool TConfig::UserRegionsInTitles() const {
    if (Impl->Exps.ReverseTitlesExp) {
        return false;
    }
    return Impl->Exps.UserRegionsInTitles;
}

TString TConfig::GetRelevRegionName() const {
    return Impl->RP.RelevRegionName;
}

bool TConfig::MnRelevRegion() const {
    return Impl->Exps.MnRelevRegion;
}

float TConfig::MnRelevRegionThreshold() const {
    return Impl->Exps.MnRelevRegionThreshold;
}

// SNIPPETS-2713
bool TConfig::TunedForumClassifier() const {
    return Impl->Exps.TunedForumClassifier;
}

// SNIPPETS-1874
bool TConfig::ShuffleWords() const {
    return Impl->Exps.Snippets1874;
}
bool TConfig::BrkBold() const {
    return !Impl->Exps.KpBold;
}
bool TConfig::BrkReg() const {
    return !Impl->Exps.KpReg;
}
bool TConfig::BrkTit() const {
    return !Impl->Exps.KpTit;
}
bool TConfig::BrkGg() const {
    return !Impl->Exps.KpGg;
}
size_t TConfig::BrkPct() const {
    return 100 - Impl->Exps.KpPct;
}
size_t TConfig::BrkProb() const {
    return 100 - Impl->Exps.KpProb;
}

// SNIPPETS-2808
float TConfig::GetByLinkLen() const {
    if (UseTouchSnippetBody()) {
        return UseTurkey() ? 3.0f : 2.0f;
    }
    return UseTurkey() ? 1.0f : 0.78f;
}

// SNIPPETS-2875
bool TConfig::OpenGraphInTitles() const {
    if (Impl->Exps.ReverseTitlesExp) {
        return false;
    }
    return Impl->Exps.OpenGraphInTitles;
}

bool TConfig::UseTurkishReadabilityThresholds() const {
    return UseTurkey() || Impl->Exps.ReverRusTrash;
}

// SNIPPETS-2923
bool TConfig::UseExtSnipRowLimit() const {
    return Impl->Exps.UseExtSnipRowLimit;
}

ui32 TConfig::GetExtSnipRowLimit() const {
    return Impl->Exps.ExtSnipRowLimit;
}

// SNIPPETS-2933
bool TConfig::EraseBadSymbolsFromTitle() const {
    if (Impl->Exps.ReverseTitlesExp) {
        return false;
    }
    return Impl->Exps.EraseBadSymbolsFromTitle;
}

bool TConfig::IsTouchReport() const {
    if (IsNeedExt() && GetReport().empty()) {
        return true;
    }
    return GetReport() == "www-touch";
}

bool TConfig::IsPadReport() const {
    return GetReport() == "www-tablet";
}

float TConfig::GetRowScale() const {
    if (UseTouchSnippetBody()) {
        return Impl->Exps.TouchRowScale;
    }
    if (Impl->Exps.UsePadSnippets && IsPadReport()) {
        return Impl->Exps.PadRowScale;
    }
    return 1.0f;
}

const TString& TConfig::GetReport() const {
    return Impl->Exps.Report ? Impl->Exps.Report : Impl->RP.Report;
}

bool TConfig::IsWebReport() const {
    return GetReport() == "www";
}

bool TConfig::AddGenericSnipForMobile() const {
    return Impl->Exps.GenericForMobile;
}

bool TConfig::QuotesInYaca() const {
    return Impl->Exps.QuotesInYaca;
}

bool TConfig::ShortYacaTitles() const {
    return Impl->Exps.ShortYacaTitles && IsTouchReport() && GetMainBna();
}

size_t TConfig::ShortYacaTitleLen() const {
    return Impl->Exps.ShortYacaTitleLen;
}

// SNIPPETS-3120
bool TConfig::SuppressCyrForSpok() const {
    return Impl->Exps.SuppressCyrForSpok;
}

// SNIPPETS-5345
size_t TConfig::AllowedTitleEmojiCount() const {
    return Impl->Exps.AllowedTitleEmojiCount;
}

size_t TConfig::AllowedSnippetEmojiCount() const {
    return Impl->Exps.AllowedSnippetEmojiCount;
}

// BLNDR-5599
size_t TConfig::AllowedExtSnippetEmojiCount() const {
    return Impl->Exps.AllowedExtSnippetEmojiCount;
}

bool TConfig::ShouldCanonizeUnicode() const {
    return Impl->Exps.ShouldCanonizeUnicode;
}


// SNIPPETS-3429

bool TConfig::IsRelevantQuestionAnswer() const {
    float factorValue = 0.f;
    if (!GetRankingFactor(FI_TEXT_HEAD, factorValue) || factorValue < 0.4f) {
        return false;
    }
    if (!GetRankingFactor(FI_TEXT_BM25, factorValue) || factorValue < 0.5f) {
        return false;
    }
    return true;
}

bool TConfig::QuestionTitles() const {
    if (IsTouchReport()) {
        return false;
    }
    return Impl->Exps.QuestionTitles ||
           QuestionTitlesApproved();
}

bool TConfig::QuestionTitlesApproved() const {
   if (Impl->Exps.QuestionTitlesApproved) {
       return true;
   }
   if (Impl->Exps.QuestionTitlesRelevant &&
       IsRelevantQuestionAnswer())
   {
       return true;
   }
   return false;
}

bool TConfig::QuestionSnippets() const {
    return Impl->Exps.QuestionSnippetsApproved ||
           QuestionSnippetsRelevant();
}

bool TConfig::QuestionSnippetsRelevant() const {
    return Impl->Exps.QuestionSnippetsRelevant &&
           IsRelevantQuestionAnswer();
}

float TConfig::QuestionSnippetLen() const {
    return Impl->Exps.QuestionSnippetLen;
}

bool TConfig::BigMediawikiSnippet() const {
    return Impl->Exps.BigMediawikiSnippet;
}

// SNIPPETS-3517
float TConfig::BigMediawikiSnippetLen() const {
    return IsTouchReport() ?
           Impl->Exps.BigMediawikiTouchSnippetLen :
           Impl->Exps.BigMediawikiSnippetLen;
}

// FACTS-1002
float TConfig::BigMediawikiReadMoreLenMultiplier() const {
    return Impl->Exps.BigMediawikiReadMoreLenMultiplier;
}

// FACTS-920
float TConfig::BigMediawikiQualityFilterMultiplier() const {
    return Impl->Exps.BigMediawikiQualityFilterMultiplier;
}

// SNIPPETS-3037
bool TConfig::UseYoutubeChannelImage() const {
    return Impl->Exps.UseYoutubeChannelImage;
}

// SNIPPETS-3227
bool TConfig::UseMobileUrl() const {
    return Impl->Exps.UseMobileUrl;
}
bool TConfig::HideUrlMenuForMobileUrl() const {
    return Impl->Exps.HideUrlMenuForMobileUrl;
}

bool TConfig::UseProductOfferImage() const {
    return Impl->Exps.UseProductOfferImage;
}

// SNIPPETS-3454
bool TConfig::ChooseLongestSnippet() const {
    return Impl->Exps.ChooseLongestSnippet;
}

bool TConfig::IsCommercialQuery() const {
    float factor = 0.f;
    GetRankingFactor(FI_QUERY_COMMERCIALITY_MX, factor);
    return factor > 0.3f;
}

bool TConfig::SkipTouchSnippets() const {
    if (!IsTouchReport()) {
        return true;
    }
    return Impl->Exps.FilterCommercialQueries && IsCommercialQuery();
}


bool TConfig::UseTouchSnippetBody() const {
    return Impl->Exps.UseTouchSnippetBody && !SkipTouchSnippets();
}

bool TConfig::UseTouchSnippetTitle() const {
    return Impl->Exps.UseTouchSnippetTitle && !SkipTouchSnippets();
}

bool TConfig::ExpFlagOn(const TString& flag) const {
    return Impl->Exps.FlagsOn.contains(flag);
}

bool TConfig::ExpFlagOff(const TString& flag) const {
    return Impl->Exps.FlagsOff.contains(flag);
}

const TAnswerModels* TConfig::GetAnswerModels() const {
    return Impl->AnswerModels;
}

const THostStats* TConfig::GetHostStats() const {
    return Impl->HostStats;
}

const NNeuralNetApplier::TModel* TConfig::GetRuFactSnippetDssmApplier() const {
    return Impl->RuFactSnippetDssmApplier;
}

const NNeuralNetApplier::TModel* TConfig::GetTomatoDssmApplier() const {
    return Impl->TomatoDssmApplier;
}

const TConfig::TRelevParams& TConfig::GetRelevParams() const {
    return Impl->RP.RelevParams;
}

// SNIPPETS-5808
bool TConfig::TopCandidatesRequested() const {
    return Impl->Exps.FlagsOn.contains("add_top_candidates");
}
size_t TConfig::TopCandidateCount() const {
    return Impl->Exps.TopCandidateCount;
}
size_t TConfig::FactSnipTopCandidateCount() const {
        return Impl->Exps.FactSnipTopCandidateCount;
}
bool TConfig::SkipAlgoInTopCandidateDump(const TString& algoName) const {
    if (Impl->Exps.TopCandidateTakenAlgos.empty()) {
        // blacklist
        return Impl->Exps.TopCandidateSkippedAlgos.contains(algoName);
    } else {
        // whitelist
        return !Impl->Exps.TopCandidateTakenAlgos.contains(algoName);
    }
}
const TFactorsToDump& TConfig::TopCandidateFactorsToDump() const {
    return Impl->Exps.TopCandidateFactorsToDump;
}

// SNIPPETS-4648
int TConfig::GetSnipWidth() const {
    return Impl->Exps.SnipWidth ? Impl->Exps.SnipWidth : Impl->RP.SnipWidth;
}

// SNIPPETS-7241
ui32 TConfig::GetFormulaDegradeThreshold() const {
    return Impl->Exps.FormulaDegradeThreshold;
}

// SNIPPETS-7243
ui32 TConfig::GetAppendRandomWordsCount() const {
    return Impl->Exps.AppendRandomWordsCount;
}

// SNIPPETS-8196
bool TConfig::IsEU() const {
    const TString& tld = GetTLD();
    return (tld == "fi" || tld == "pl" || tld == "lt" || tld == "lv" || tld == "ee");
}

bool TConfig::SuppressCyr() const {
    if (IsEU()) {
        return !ExpFlagOff("suppress_cyr");
    } else {
        return ExpFlagOn("suppress_cyr");
    }
}

bool TConfig::UseMediawiki() const {
    if (IsEU()) {
        return !ExpFlagOff("use_mediawiki");
    }
    return true;
}

size_t TConfig::ForceCutSnip() const {
    return Impl->Exps.ForceCutSnip;
}

//SNIPPETS-8827
bool TConfig::AllowBreveInTitle() const {
    return Impl->Exps.AllowBreveInTitle;
}

};
