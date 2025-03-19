#pragma once

#include <util/system/defaults.h>
#include <util/generic/array_ref.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/memory/pool.h>

#include <kernel/hitinfo/hitinfo.h>
#include <kernel/doom/hits/panther_hit.h>
#include <kernel/factor_slices/factor_slices.h>
#include <kernel/factor_slices/factor_domain.h>
#include <kernel/search_daemon_iface/basetype.h>
#include <kernel/search_daemon_iface/relevance_type.h>
#include <kernel/search_types/filtration.h>
#include <kernel/search_types/search_types.h>

// Do not include any other arcadia libraries

class TBaseSearch;
class TBaseCommonSearch;
class ISearchContext;
class TSearchDetailContext;
class IPassageContext;
class IFetchDocDataContext;
class IFetchAttrsContext;
class TCgiParameters;
class TRequestParams;
class TSearcherProps;
struct ICreatorOfCustomDynamicFeaturesCalcer;
struct TRelevanceInitInfo;
struct IGatorRegister;
struct TCreateGatorsContext;
class TFactorStorage;
class TWeighedDoc;
class TSearchConfig;
class TMemoryPool;
class TBaseIndexData;
class TIndexAccessors;
struct TBaseIndexDataOptions;
class TBufferStream;
class IOutputStream;
struct TDebugHandler;
struct TAllDocPositions;
class TMemoryPool;
struct TAnnFeaturesParams;
class IDocIdStats;
class TSentenceLengthsReader;
class IThreadPool;
class TL1RelevanceContext;

namespace NMetaProtocol {
    class TDocument;
}

namespace NMetaProtocol
{
class TReport;
}

class TSearcherProps;
class IIndexData;
class IRelevance;
class IFactorsInfo;
class IExtResultsInfo;
class IExternalPantherSeacher;
struct TCalcFactorsContext;
struct TCalcRelevanceContext;
struct TCalcExtRelevanceContext;
struct TSnippetsHitsContext;
class IDocTimeCalcer;
class TBaseArchiveDocInfo;
struct TBaseArchiveDocInfoParams;
class TBaseIndexStaticData;
class TBaseIndexDynamicData;
struct IInfoRequestPrinter;
class IUrlIdInfo;
struct TDocRelevPerQueryParams;
struct TFillPerQueryParamsContext;
class TSearchContext;
class TRequestResults;
class IAttributeWriter;
class TDocumentCheater;
class TSearchErfFactorDistribution;
struct TFetchYndexContext;
struct TCalcFilteringResultsContext;
class TRemapTable;
class TUserDataManager;
struct TSumRelevParams;
class TTextMachineFeaturesCalcer;
struct TTextMachineDocHitsProviderInfo;
struct TRecommendationsSearchContext;
class TCategSeries;

namespace NFormulaBoost::NLuaWeb {
    class TLuaBoostArguments;
}

namespace NTextMachineProtocol {
    class TPbHits;
    class TPbLimitsMonitor;
    class TPbQueryWordStats;
}

namespace NFactorSlices {
    class TSlicesMetaInfo;
}

namespace NGroupingAttrs {
    class TDocsAttrs;
}

namespace NPanther {
    class TMatchedDoc;
}

class IMinimalSearch {
public:
    virtual ~IMinimalSearch() = default;

    virtual IIndexData* GetIndexData() = 0;
    virtual const IFactorsInfo* GetFactorsInfo() const = 0;
    virtual void PrepareIdxOpsSlices(NFactorSlices::TSlicesMetaInfo& /*metaInfo*/) const {
    }
    virtual IRelevance* CreateRelevance(const TBaseIndexData& baseIndexData, TIndexAccessors& accessors, const TRequestParams* rp) const = 0;

    virtual const TString& GetDefaultGroupingAttribute() const {
        return Default<TString>();
    }

    // called before TreatCgiParam
    virtual void PrepareRequestParams(TRequestParams* /*rp*/) const {
    }

    // called after TreatCgiParam
    virtual void TuneRequestParams(TRequestParams* rp) const = 0;

    // If returns non-zero, docs that don't have a group categ set will get one equal to <docid> + <return value>
    // This is to emulate sparse groupings, where some docs are grouped and most are not,
    // without actually storing anything for the ungrouped docs.
    virtual ui64 AutoGroupCategBase() const {
        return 0;
    }

    // this method is added for correct emulation of websearch under ExternalSearch
    virtual bool NeedWebBehavior() const {
        return false;
    }

    virtual const IUrlIdInfo* GetUrlIdInfo() const {
        return nullptr;
    }

    virtual void InitDynamicRankingModels(const TString& /*modelsArchive*/, const TVector<TString>& /*subDirs*/ = {}) {
    }
};

class IExternalSearch: public IMinimalSearch {
public:
    virtual const IFactorsInfo* GetWebFactorsInfo() const {
        return GetFactorsInfo();
    }

    virtual bool ProcessInfoRequest(const TRequestParams& /*rp*/, TBufferStream& /*outputStream*/) const {
        return false;
    }

    virtual bool UseMemoryPool() const {
        return true;
    }

    virtual void SerializeFirstStageAttributes(ui32 docId, TArrayRef<const char * const> attrNames, IAttributeWriter& write) const {
        for (const auto& attrName: attrNames) {
            SerializeFirstStageAttribute(docId, attrName, write);
        }
    }

    virtual void SerializeFirstStageAttribute(ui32 /*docId*/, const char* /*attrName*/, IAttributeWriter& /*write*/) const {
    }

    virtual void FillCustomProperties(TSearcherProps* /*props*/, const TRequestParams* /*rp*/) const {
    }

    virtual ISearchContext* CreateSearchContext(const TBaseSearch& /*owner*/) const {
        return nullptr;
    }
    virtual IPassageContext* CreatePassageContext(const TBaseSearch& /*owner*/) const {
        return nullptr;
    }
    virtual IFetchDocDataContext* CreateFetchDocDataContext(const TBaseSearch& /*owner*/) const {
        return nullptr;
    }
    virtual IFetchAttrsContext* CreateFetchAttrsContext(const TBaseSearch& /*owner*/) const {
        return nullptr;
    }

    virtual bool OverrideStandardReport(const TRequestParams& /*rp*/, NMetaProtocol::TReport& /*protoData*/, TBufferStream& /*out*/) const {
        return false;
    }

    virtual bool IsRealsearch(const char* /*className*/) const {
        return false;
    }

    virtual void PostCreateSearch(TBaseCommonSearch& /*search*/, const char* /*className*/) {
    }

    virtual void Init(const TSearchConfig* /*config*/) {
    }

    virtual void PrintInfo(TStringBuf /*params*/, IOutputStream& /*out*/) {
    }

    virtual void TuneIndexDocIdsFraction(double* /*fraction*/) const {
    }

    virtual TString GetDynamicModelsFileMD5() const {
        return {};
    }

    virtual TString GetDynamicModelsSvnVersion() const {
        return {};
    }

    virtual TString GetDynamicModelsSandboxTaskId() const {
        return {};
    }

    virtual TString GetDynamicModelsSandboxResourceId() const {
        return {};
    }

    virtual TString GetDynamicModelsBuildTime() const {
        return {};
    }

    virtual TString TryGetDynamicModelsFileMD5(IThreadPool&) const {
        return GetDynamicModelsFileMD5();
    }
};

class IIndexData {
public:
    virtual ~IIndexData() = default;

    virtual TString ExternalIndexType() { return "unknown"; }

    virtual void Open(const char* indexDir, bool isPolite, const TSearchConfig* config) = 0;
    virtual void Close() = 0;

    virtual void SetIndexDataOptions(TBaseIndexDataOptions& /*options*/) const {
    }

    virtual TUserDataManager* CreateUserDataManager(const char* /*indexName*/) {
        return nullptr;
    }

    virtual TBaseIndexStaticData* CreateBaseIndexStaticData(
            const TSearchConfig& /*config*/,
            const TBaseIndexDataOptions& /*options*/)
    {
        return nullptr;
    }
    virtual TBaseIndexDynamicData* CreateBaseIndexDynamicData(
            const TSearchConfig& /*config*/,
            const TBaseIndexDataOptions& /*options*/)
    {
        return nullptr;
    }

    virtual IDocIdStats* CreateDocIdStats(ui32 /*docCount*/) {
        return nullptr;
    }

    virtual NGroupingAttrs::TDocsAttrs* CreateDocsAttrs(EBaseType, bool, const char*, bool lockMemory = false) {
        Y_UNUSED(lockMemory);
        return nullptr;
    }
};


class IRelevance {
public:
    IRelevance()
        : CheatRelevanceOverriden(true)
    {
    }
    virtual ~IRelevance() = default;

    virtual bool IsRecommendationsRequest() const {
        return false;
    }

    virtual void RecommendationsSearch(TRecommendationsSearchContext* /*ctx*/) {}

    // this is only for geosearch because it doesn't have IExternalSearch
    virtual const IFactorsInfo* GetWebFactorsInfo() const {
        return GetFactorsInfo();
    }
    virtual const IFactorsInfo* GetFactorsInfo() const = 0;

    // this method is added for correct emulation of websearch under ExternalRelevance
    virtual bool NeedWebBehavior() const {
        return false;
    }

    virtual bool GetSwitchType(ESwitchType& /*switchType*/) const {
        return false;
    }

    virtual void FillBoostArguments(NFormulaBoost::NLuaWeb::TLuaBoostArguments& /*args*/) const {
    }

    // called after TreatCgiParam
    // use IExternalSearch::TuneRequestParams when possible
    // this method only for per request features
    virtual void PreInitRequest(TRequestParams* /*rp*/) {
    }

    virtual void PostInitRequest(const TRelevanceInitInfo& /*initInfo*/) {
    }

    virtual void CreateGators(TCreateGatorsContext& /*ctx*/) const {
    }

    virtual bool AcceptDoc(ui32 /*docId*/) const {
        return true;
    }

    virtual bool ShouldApplyAcceptRestrictedDoc() const {
        return false;
    }
    virtual bool AcceptRestrictedDoc(ui32 /*docId*/) const {
        return true;
    }

    virtual bool AcceptDocWithHits(const TAllDocPositions& /*doc*/) const {
        return true;
    }

    virtual bool AcceptDocWithHitsBeforeAdd(const TAllDocPositions& /*doc*/) const {
        return true;
    }

    virtual ICreatorOfCustomDynamicFeaturesCalcer* CreateCustomDynamicFeaturesCalcerCreator() {
        return nullptr;
    }

    virtual void CalcFactors(TCalcFactorsContext& ctx) = 0;

    virtual const char* GetFormulaName() {
        return nullptr;  // to be shown in search results
    }
    virtual void CalcRelevance(TCalcRelevanceContext& ctx) = 0;

    // Used for destroying members created after Relevance Ctor
    virtual void PreFinalizeRequest() {
    }

    // Used in info-requests
    virtual void FillRankModel(TDebugHandler* /*debugHandler*/) const {
    }

    virtual void OnBeforeSecondPass() {
    }

    bool IsCheatRelevanceOverriden() {
        CheatRelevance(nullptr, 0);
        return CheatRelevanceOverriden;
    }

    //! Called after relevance for _all_ docs in response is calculated
    virtual void CheatRelevance(TWeighedDoc* /*docs*/, size_t /*count*/) {
        CheatRelevanceOverriden = false;
    }
    // Overriding this function allows you to filter out some search results after their relevance is calculated.
    virtual bool ShouldRemoveDocument(const TWeighedDoc& /*document*/) {
        return false;
    }
    virtual bool ShouldRemoveDocument(TCalcFactorsContext& /*ctx*/, TSumRelevParams& /*srParams*/) {
        return false;
    }

    virtual void FillProperties(TSearcherProps* /*props*/) const {
    }
    virtual TVector<const char*> SerializeAttributes(const TWeighedDoc& doc, TArrayRef<const char * const> attrNames, IAttributeWriter& write, const TRequestResults* results) const {
        TVector<const char*> result(Reserve(attrNames.size()));
        for (const auto& attrName: attrNames) {
            if (!SerializeAttribute(doc, attrName, write, results)) {
                result.push_back(attrName);
            }
        }
        return result;
    }
    virtual TVector<const char*> SerializeFirstStageAttributes(const TWeighedDoc& doc, TArrayRef<const char * const> attrNames, IAttributeWriter& write, const TRequestResults* results) const {
        TVector<const char*> result(Reserve(attrNames.size()));
        for (const auto& attrName: attrNames) {
            if (!SerializeFirstStageAttribute(doc, attrName, write, results)) {
                result.push_back(attrName);
            }
        }
        return result;
    }
    virtual bool SerializeAttribute(const TWeighedDoc& /*doc*/, const char* /*attrName*/, IAttributeWriter& /*write*/, const TRequestResults* /*results*/) const {
        return false;
    }
    virtual bool SerializeFirstStageAttribute(const TWeighedDoc& /*doc*/, const char* /*attrName*/, IAttributeWriter& /*write*/, const TRequestResults* /*results*/) const {
        return false;
    }

    // if context is NULL then this method returns only flag, not fills result
    virtual bool SerializeDocProperty(const TSearchDetailContext* /*context*/, ui32 /*hndl*/, const char* /*prop*/, unsigned /*index*/, TString& /*res*/) const {
        return false;
    }

    virtual bool ExtractSnippetsHits(ui32 /*docId*/, const TSnippetsHitsContext* /*shc*/) {
        return true;
    }

    virtual IExtResultsInfo* GetExtResultsInfo() {
        return nullptr;
    }

    virtual IDocTimeCalcer* CreateDocTimeCalcer() {
        return nullptr;
    }

    virtual TBaseArchiveDocInfo* CreateArchiveAccessor(const TBaseArchiveDocInfoParams& /*params*/) {
        return nullptr;
    }

    virtual IInfoRequestPrinter* CreateInfoRequestPrinter(const TString& /*type*/) {
        return nullptr;
    }

    virtual void FillPerQueryParams(TFillPerQueryParamsContext& /*ctx*/) const {
    }

    virtual void PreSearch(TSearchContext& /*ctx*/) {
    }

    // Deprecated: use CalcFilteringResults instead
    virtual bool DoCustomFetchYndex(TFetchYndexContext& /*ctx*/) {
        return false;
    }

    virtual bool CalcFilteringResults(TCalcFilteringResultsContext& /*ctx*/) {
        return false;
    }

    virtual void CollectFactorDistribution(TSearchErfFactorDistribution* /*erfFactorDistr*/,
            const TFactorStorage& /*factorStorage*/, const ui32 /*docId*/, const ui64 /*reqIdTimestamp*/) {
    }

    virtual const char* GetTimeErfField() const {
        return "Mtime";
    }

    virtual int GetErfFieldIndex(const char* /*factorName*/) const {
        return -1;
    }

    virtual const TRemapTable* GetTrRemapTable(int /*numHits*/) const {
        return nullptr;
    }

    virtual bool UseWebDocQuorum() const {
        return true;
    }

    virtual bool NeedWebBehaviorL1() const {
        return false;
    }

    virtual bool ShouldDoublePruningTargetDocs() const {
        return true;
    }

    virtual bool UseVirtualGeoHits() const {
        return true;
    }

    // true means that the CalcFactors() creates an incorrect FactorDomain (legacy behavior). If so, then assignments of factors
    // should only happen in the IExternalRelevanceCalcFactorsHelper routines, or before the first CalcFactors() call
    virtual bool UsesVirtualFactorDomain() const {
        return false;
    }

    virtual const TMap<ui32, NTextMachineProtocol::TPbHits>* GetTextMachineDocHits() const {
        return nullptr;
    }

    virtual TString GetSerializedQBundle() const {
        return {};
    }

    virtual const TMap<ui32, NTextMachineProtocol::TPbLimitsMonitor>* GetTextMachineDocLimitsMonitors() const {
        return nullptr;
    }

    virtual const THashMap<ui32, NTextMachineProtocol::TPbQueryWordStats>* GetTextMachineDocWordStats() const {
        return nullptr;
    }

    virtual void SetL1SlicesMetaInfo(NFactorSlices::TSlicesMetaInfo& /*metaInfo*/) const {
    }

    virtual void PrepareBasesearchSlices(NFactorSlices::TSlicesMetaInfo& /*metaInfo*/) {
    }

    // Документы, подтасовываемые к пантерному поиску
    // Дополнительно пройдут фильтрацию по имеющимся в запросе литералам (фильтрующим)
    virtual TVector<ui32> CalcExtraFilteringDocsForPanther() {
        return {};
    }

    // Документы добавляемые после прохождения пантерного поиска
    virtual TVector<ui32> CalcExtraFilteringDocs() {
        return {};
    }

    virtual void AddExtraPantherDocs(TVector<NPanther::TMatchedDoc>& /*documents*/) {
    }

    virtual IExternalPantherSeacher* GetExternalPantherSeacher() {
        return nullptr;
    }

    virtual void L1SelectDocs(TVector<NPanther::TMatchedDoc>& /*documents*/, const TL1RelevanceContext& /*ctx*/) {
    }

    virtual TTextMachineFeaturesCalcer* CreateExternalTextMachineCalcer(const bool /*needOpenIterators*/) {
        return nullptr;
    }

    virtual void AddExtraPantherTerms(TVector<TString>& /*extraTerms*/) {
    }

    virtual bool NeedPantherBoost() const {
        return false;
    }

    virtual void UpdatePantherRelevance(TArrayRef<const ui32> /*docIds*/, TArrayRef<ui32> /*relevances*/) const {
    }

    virtual bool DocCategs(ui32 /*docid*/, ui32 /*attrnum*/, TCategSeries& /*result*/) const {
        return false;
    }

    virtual TString GetShardWithoutTimestamp(const TString& shardName) const {
        Y_UNUSED(shardName);
        ythrow yexception() << "newest runtime not supported";
    }

    virtual bool AcceptDocForNewRt() const {
        return false;
    }

    virtual bool CalculateVirtualGroupAttr(ui32 /*docId*/, ui32 /*virtualAttrIndex*/, TCateg& /*value*/) {
        return false;
    }

private:
    bool CheatRelevanceOverriden;
};

struct TFmlParam;

class IExtResultsInfo {
public:
    virtual ~IExtResultsInfo() = default;

    virtual void InitExtResults(const TFmlParam** fmlParams, size_t count) = 0;
    virtual void CalcExtRelevance(TCalcExtRelevanceContext& ctx) = 0;
};

class IExternalPantherSeacher {
public:
    virtual ~IExternalPantherSeacher() = default;

    virtual TVector<TVector<NDoom::TPantherHit>> GetExternalPanther(const TVector<TString>& keys) const = 0;
};

class ITRIterator;
class TBasesearchErfManager;
class TUserDataResultsIter;
namespace NGroupingAttrs {
    class TDocsAttrs;
}
class IFiltrationModel;

struct TRelevanceInitInfo {
    const ITRIterator* TRIterator = nullptr;
    const TBasesearchErfManager* ErfManager = nullptr;
    const TRequestParams* RP = nullptr;
    const TDocumentCheater* DocumentCheater = nullptr;
    const NGroupingAttrs::TDocsAttrs* DocsAttrs = nullptr;
    const TAnnFeaturesParams* AnnFeaturesParams = nullptr;
    TUserDataResultsIter* UserDataResultsIter = nullptr;
    TMemoryPool* Pool = nullptr;
    float Quorum = 0.0;
    const IFiltrationModel* FiltrationModel = nullptr;

    TRelevanceInitInfo(
            const ITRIterator* trIterator,
            const TBasesearchErfManager* erfManager,
            const TRequestParams* rp,
            const TDocumentCheater* documentCheater = nullptr,
            const NGroupingAttrs::TDocsAttrs* docsAttrs = nullptr,
            const TAnnFeaturesParams* annFeaturesParams = nullptr,
            TUserDataResultsIter* userDataResultsIter = nullptr,
            TMemoryPool* pool = nullptr,
            float quorum = 0.0,
            const IFiltrationModel* filtrationModel = nullptr)
        : TRIterator(trIterator)
        , ErfManager(erfManager)
        , RP(rp)
        , DocumentCheater(documentCheater)
        , DocsAttrs(docsAttrs)
        , AnnFeaturesParams(annFeaturesParams)
        , UserDataResultsIter(userDataResultsIter)
        , Pool(pool)
        , Quorum(quorum)
        , FiltrationModel(filtrationModel)
    {
    }
};

struct TCreateGatorsContext {
    IGatorRegister* GatorRegister;
    TMemoryPool& Pool;

   const float* WordWeight;
   size_t WordWeightCount;

   TCreateGatorsContext(IGatorRegister* gatorRegister,
        TMemoryPool& pool,
        const float* wordWeight,
        size_t wordWeightCount
    )
       : GatorRegister(gatorRegister)
       , Pool(pool)
       , WordWeight(wordWeight)
       , WordWeightCount(wordWeightCount)
    {}
};

struct TDocumentHitsBuf;

struct IExternalRelevanceCalcFactorsHelper {
    virtual ~IExternalRelevanceCalcFactorsHelper() = default;

    virtual void CalcNorelevFeatures(TFactorStorage& fs) = 0;
    virtual void CalcFastFeatures(TFactorStorage& fs) = 0;
    virtual void CalcAllFeatures(TFactorStorage& fs) = 0;
    virtual void CalcRefineFactor(TFactorStorage& fs, const TDocumentHitsBuf& trHits, const TDocumentHitsBuf& lrHits, bool fast) = 0;
    virtual void AggregateWebMetaFeatures(TFactorStorage& fs) = 0; // TODO: const TFactorStorage& fs

    // see IRelevance::CreateCustomDynamicFeaturesCalcerCreator
    virtual void CalcDynamicFeatures() = 0;
};

struct IExternalRelevanceCalcRelevanceHelper {
    virtual ~IExternalRelevanceCalcRelevanceHelper() = default;

    virtual void CalcWebFormula(TFactorStorage** fs, float* res, TRelevance* resRaw, size_t count, bool fast) = 0; // TODO: const TFactorStorage** fs
    virtual void CalcExtFormula(size_t ext, TSumRelevParams* params, TRelevance* res, size_t count, TVector<size_t>* indexes) = 0;
};



struct TCalcFactorsContext {
    bool Fast;
    bool NorelevOnly;
    ui32 DocId;
    TFactorStorage* Factors;  // factors are cleared by basesearch before CalcFactors
    const TDocumentHitsBuf* TextHits;
    const TDocumentHitsBuf* LinkHits;
    const TDocumentHitsBuf* AnnotationHits;
    const TDocRelevPerQueryParams* DocRelevPerQueryParams;
    IExternalRelevanceCalcFactorsHelper* Helper;

    TCalcFactorsContext(
        bool fast,
        ui32 docId,
        TFactorStorage* factors,
        const TDocumentHitsBuf* textHits,
        const TDocumentHitsBuf* linkHits,
        const TDocumentHitsBuf* annotationHits,
        const TDocRelevPerQueryParams* docRelevPerQueryParams,
        IExternalRelevanceCalcFactorsHelper* helper
    )
        : Fast(fast)
        , NorelevOnly(false)
        , DocId(docId)
        , Factors(factors)
        , TextHits(textHits)
        , LinkHits(linkHits)
        , AnnotationHits(annotationHits)
        , DocRelevPerQueryParams(docRelevPerQueryParams)
        , Helper(helper)
    {
    }
};

struct TCalcRelevanceContext {
    bool Fast;

    size_t Count;
    TSumRelevParams* Params;
    float* ResultRelev;
    TRelevance* ResultRelevRaw;

    bool UseResultRelevRaw;
    IExternalRelevanceCalcRelevanceHelper* Helper;

    TCalcRelevanceContext(bool fast, TSumRelevParams* params, float* resultRelev, size_t count, IExternalRelevanceCalcRelevanceHelper* helper, TRelevance* resultRelevRaw)
        : Fast(fast)
        , Count(count)
        , Params(params)
        , ResultRelev(resultRelev)
        , ResultRelevRaw(resultRelevRaw)
        , UseResultRelevRaw(false)
        , Helper(helper)
    {
    }
};

struct TCalcExtRelevanceContext {
    size_t Ext;

    size_t Count;
    TSumRelevParams** Params;
    float* ResultRelev;
    TRelevance* ResultRelevRaw;

    bool UseResultRelevRaw;
    IExternalRelevanceCalcRelevanceHelper* Helper;

    TCalcExtRelevanceContext(size_t ext, float* resultRelev, size_t count, IExternalRelevanceCalcRelevanceHelper* helper, TRelevance* resultRelevRaw, TSumRelevParams** params)
        : Ext(ext)
        , Count(count)
        , Params(params)
        , ResultRelev(resultRelev)
        , ResultRelevRaw(resultRelevRaw)
        , UseResultRelevRaw(false)
        , Helper(helper)
    {
    }
};

class IDocTimeCalcer {
public:
    virtual ~IDocTimeCalcer() = default;

    virtual time_t GetTime(ui32 docid) = 0;
};

class IDocProcessor {
public:
    virtual ~IDocProcessor() = default;

    virtual void Process(ui32 docId, const NGroupingAttrs::TDocsAttrs& da, const TAllDocPositions& doc, const TTRIteratorHitInfos* hitInfos = nullptr) = 0;
    virtual bool DocumentBanned(ui32 docId) const = 0;
    virtual void SerializeFirstStageAttributes(ui32 docId, TArrayRef<const char * const> attrNames, IAttributeWriter& write) {
        for (const auto& attrName: attrNames) {
            SerializeFirstStageAttribute(docId, attrName, write);
        }
    }
    virtual void SerializeFirstStageAttribute(ui32 /*docId*/, const char* /*attrName*/, IAttributeWriter& /*write*/) {}

    virtual bool IsBatchProcessor() const {
        return false;
    }

    virtual void ProcessBatch(TArrayRef<const ui32> /*docIds*/, const NGroupingAttrs::TDocsAttrs& /*da*/) {}
};

class IReportDocumentProcessor {
public:
    virtual ~IReportDocumentProcessor() = default;

    virtual void Process(const TRequestParams& rp, NMetaProtocol::TDocument* document) = 0;
};

class IUrlIdInfo {
public:
    virtual ~IUrlIdInfo() = default;

    virtual ui64 GetUrlId(ui32 docId) const = 0;
};

class IDocFactorCalcer: public TPoolable {
public:
    virtual ~IDocFactorCalcer() = default;
    virtual void CalcFactors(TCalcFactorsContext& ctx) = 0;
};
