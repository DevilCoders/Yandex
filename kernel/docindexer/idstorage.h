#pragma once

#include <ysite/directtext/sentences_lens/sentenceslensproc.h>

#include <kernel/indexer/attrproc/attrproc.h>
#include <kernel/indexer/baseproc/archflush.h>
#include <kernel/indexer/baseproc/baseconf.h>
#include <kernel/indexer/baseproc/docprocessor.h>
#include <kernel/indexer/baseproc/groupproc.h>
#include <kernel/indexer/direct_text/dt.h>
#include <kernel/indexer/faceproc/docattrs.h>
#include <kernel/indexer/face/docinfo.h>
#include <kernel/indexer/parseddoc/pdstorage.h>
#include <kernel/indexer/tfproc/tfproc.h>
#include <kernel/queryfactors/querybased.h>
#include <kernel/search_zone/searchzone.h>
#include <kernel/titlefeatures/titlefeatures.h>

#include <util/stream/buffer.h>

class TIndexedDocStorageFactory;
struct IYndexStorage;
class TArchiveCreator;
class TFreqCalculator;
class TCompatiblePureContainer;

namespace NRealTime {
    class TIndexedDoc;
}

namespace NIndexerCore {
    class TDater;
    class TPrewalrusProcessor;
    class TInvCreatorDTCallback;
    class TDirectIndex;
    class TDirectTextCreator;
    class TDefaultDisamber;
}

struct TIndexedDocStorageConfig
    : public TBaseConfig
    , public TTextFeaturesConfig
    , public TQueryFactorsConfig
    , public TTitleFeaturesConfig
{
    TString QueryFactorsDir;
    bool UseSentenceLengthProcessor;
    bool IsRtYServer;
    bool StripKeysOnIndex = false;

    TIndexedDocStorageConfig(const TBaseConfig& baseCfg)
        : TBaseConfig(baseCfg)
        , UseSentenceLengthProcessor(false)
        , IsRtYServer(false)
    {}

    TIndexedDocStorageConfig()
        : UseSentenceLengthProcessor(false)
        , IsRtYServer(false)
    {}
};

struct TIndexedDocStorageConstructionData{
    const TIndexedDocStorageConfig& Cfg;
    const TAttrProcessorConfig& AttrCfg;
    const ICatFilter* Filter;
    const TGroupAttributer* GroupAttributer;
    const NShop::TModel* ShopModel;
    const NShop::TWordsHolder* ShopWordHolder;
    TVector<IYndexStorage*>* AdditionalStorages;
    TAutoPtr<NIndexerCore::TParsedDocStorage> CustomParsedDocStorage;
    NIndexerCore::TDocumentProcessor::IEventsProcessor* EventsProcessor;


    TIndexedDocStorageConstructionData(
        const TIndexedDocStorageConfig& cfg,
        const TAttrProcessorConfig& attrCfg)
        : Cfg(cfg)
        , AttrCfg(attrCfg)
        , Filter(nullptr)
        , GroupAttributer(nullptr)
        , ShopModel(nullptr)
        , ShopWordHolder(nullptr)
        , AdditionalStorages(nullptr)
        , EventsProcessor(nullptr)
    {

    }
};

class TIndexedDocStorageBase {
private:
    THolder<TIndexedDocStorageFactory> StorageFactory;
    THolder<NIndexerCore::TDefaultDisamber> Disamber;
    THolder<NIndexerCore::TInvCreatorDTCallback> InvCreator;
    TBufferOutput OutArchive;
    NRealTime::TIndexedDoc* IndexedDoc = nullptr;
public:
    TIndexedDocStorageBase(TVector<IYndexStorage*>* additionalStorages, bool useDisamber, const TString& stopWordFile, bool stripKeysOnIndex = false);
    ~TIndexedDocStorageBase();

    void AddDoc(NRealTime::TIndexedDoc* indexedDoc);
    void CommitDoc();
protected:
    NIndexerCore::TInvCreatorDTCallback* GetInvCreator() {
        return InvCreator.Get();
    }
    NIndexerCore::TDefaultDisamber* GetDisamber() {
        return Disamber.Get();
    }
    TBufferOutput& GetArchiveStream() {
        return OutArchive;
    }
};

class TIndexedDocStorage : public TIndexedDocStorageBase {
private:
    THolder<NIndexerCore::TPrewalrusProcessor> PrewalrusProcessor;
    THolder<NIndexerCore::TDater> DaterProcessor;
    THolder<TBufferOutput> OutFullArchive;
    TAttrDocumentAction AttrPr;
    TGroupDocumentAction GrpAction;
    const NGroupingAttrs::TConfig* GroupingConfig;
    THolder<TTextFeaturesProcessor> TextPr;
    THolder<TQueryBasedProcessor> QueryBasedProcessor;
    THolder<TTitleFeaturesProcessor> TitleFeaturesProcessor;

    THolder<TFullArchiveFlusher> FullArchiveFlusher;
    TSentenceLensProcessor SentenceLengthProcessor;

public:
    TIndexedDocStorage(TIndexedDocStorageConstructionData& data, const TSimpleSharedPtr<TCompatiblePureContainer>* pure = nullptr);
    ~TIndexedDocStorage();

    void SetConfig(const NGroupingAttrs::TConfig* groupingConfig) {
        GroupingConfig = groupingConfig;
    }

    void AddInvCreator(NIndexerCore::IDirectTextCallback4* callback);
    void AddDTCallback(NIndexerCore::IDirectTextCallback2* callback);
    void SetParserConf(const TString& name);
    void InitAdultDetector(const TString& adultFileName, TAdultPreparat& adultPraparat, bool useprepared);
    void IndexDoc(const TDocInfoEx&, TFullDocAttrs&, NRealTime::TIndexedDoc*, const TString& url4Archive, const bool httpCutterUsage = true, const NIndexerCore::TExtraTextZones* = nullptr);
    void ClearAndFlushArchiveBuffers();

    // This method is used to eliminate TOwnerCanonizer duplication in the rtindexer.
    const TOwnerCanonizer& GetInternalOwnerCanonizer() {
        return AttrPr.GetInternalOwnerCanonizer();
    }

    static void FillAttributes(const NGroupingAttrs::TConfig* groupingConfig, const TFullDocAttrs& extAttrs, NRealTime::TIndexedDoc& indexedDoc);
    static void FillAttributesToExtAttrs(TFullDocAttrs& extAttrs, const NRealTime::TIndexedDoc& indexedDoc);
};

class TIndexedDocDirectIndexStorage : public TIndexedDocStorageBase {
private:
    THolder<NIndexerCore::TDirectTextCreator> DirectCreator;
    THolder<NIndexerCore::TDirectIndex> DirectIndex;
    THolder<TArchiveCreator> PlainArchiveCreator;
    THolder<TFreqCalculator> FreqCalculator;
public:
    TIndexedDocDirectIndexStorage(bool useLemmer);
    ~TIndexedDocDirectIndexStorage();

    NIndexerCore::TDirectIndex& GetDirectIndex();
};
