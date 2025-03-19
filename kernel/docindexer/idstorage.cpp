#include "idstorage.h"

#include <kernel/indexer/baseproc/archflush.h>
#include <kernel/indexer/baseproc/prewalrusproc.h>

#include <kernel/indexer/attrproc/attrproc.h>
#include <kernel/indexer/attrproc/catwork.h>
#include <kernel/indexer/directindex/directindex.h>
#include <kernel/indexer/directindex/extratext.h>
#include <kernel/indexer/disamber/disamber.h>
#include <kernel/indexer/posindex/invcreator.h>
#include <kernel/indexer/attrproc/attributer.h>
#include <kernel/indexer/parseddoc/pdstorage.h>

#include <ysite/directtext/dater/dater.h>
#include <ysite/directtext/textarchive/createarc.h>
#include <ysite/directtext/freqs/freqs.h>
#include <ysite/yandex/pure/pure_container.h> // for TCompatiblePureContainer

#include <yweb/protos/indexeddoc.pb.h>

#include <kernel/keyinv/invkeypos/keynames.h>
#include <kernel/keyinv/indexfile/indexstorageface.h>

using namespace NIndexerCore;


class TIndexedDocStorageFactory :  public IYndexStorageFactory {
private:
    class TIndexedDocYndexStorage : public IYndexStorage {
    public:
        TIndexedDocYndexStorage(const NRealTime::TIndexedDoc_EEntryType type)
        : Type(type)
        {}
        void StorePositions(const char* textPointer, SUPERLONG* filePositions, size_t filePosCount) override {
            NRealTime::TIndexedDoc_TEntry &docEntry = *IndexedDoc->AddEntries();
            docEntry.SetKey(textPointer);
            docEntry.SetType(Type);
            for (size_t i = 0; i < filePosCount; ++i) {
                docEntry.AddPositions((ui32)filePositions[i]); // upper dword of pos contains only docid bits
            }
            if (AdditionalStoragesPtr) {
                for (auto storagePtr: *AdditionalStoragesPtr)
                    storagePtr->StorePositions(textPointer, filePositions, filePosCount);
            }
        }
        void SetDoc(NRealTime::TIndexedDoc* indexedDoc) {
            IndexedDoc = indexedDoc;
        }
        void SetAdditionalStorages(TVector<IYndexStorage*>* additionalStoragesPtr) {
            AdditionalStoragesPtr = additionalStoragesPtr;
        }
    private:
        NRealTime::TIndexedDoc_EEntryType Type;
        NRealTime::TIndexedDoc* IndexedDoc = nullptr;
        TVector<IYndexStorage*>* AdditionalStoragesPtr = nullptr;
    };
public:
    TIndexedDocStorageFactory(TVector<IYndexStorage*>* additionalStorages)
        : LemmDocStorage(NRealTime::TIndexedDoc_EEntryType_Word)
        , AttrDocStorage(NRealTime::TIndexedDoc_EEntryType_Attribute)
    {
        if (additionalStorages) {
            AdditionalStorages = *additionalStorages;
            LemmDocStorage.SetAdditionalStorages(&AdditionalStorages);
            AttrDocStorage.SetAdditionalStorages(&AdditionalStorages);
        }
    }
    void SetDoc(NRealTime::TIndexedDoc* indexedDoc) {
        LemmDocStorage.SetDoc(indexedDoc);
        AttrDocStorage.SetDoc(indexedDoc);
    }
    void GetStorage(IYndexStorage ** ret, IYndexStorage::FORMAT format) override {
        if (format == IYndexStorage::PORTION_FORMAT_ATTR) {
            *ret = &AttrDocStorage;
        } else {
            *ret = &LemmDocStorage;
        }
    }
    void ReleaseStorage(IYndexStorage *) override {
    }
private:
    TVector<IYndexStorage*> AdditionalStorages;
    TIndexedDocYndexStorage LemmDocStorage;
    TIndexedDocYndexStorage AttrDocStorage;
};

TIndexedDocStorageBase::TIndexedDocStorageBase(TVector<IYndexStorage*>* additionalStorages, bool useDisamber, const TString& stopWordFile, bool stripKeysOnIndex/* = false*/)
    : StorageFactory(new TIndexedDocStorageFactory(additionalStorages))
    , OutArchive(1u << 14)
{
    TInvCreatorConfig invCfg(1, 0, stripKeysOnIndex);
    if (useDisamber)
        Disamber.Reset(new NIndexerCore::TDefaultDisamber());
    invCfg.StopWordFile = stopWordFile;
    invCfg.GroupForms = false;
    InvCreator.Reset(new TInvCreatorDTCallback(StorageFactory.Get(), invCfg));
}

TIndexedDocStorageBase::~TIndexedDocStorageBase() {
}

void TIndexedDocStorageBase::AddDoc(NRealTime::TIndexedDoc* indexedDoc) {
    IndexedDoc = indexedDoc;
    StorageFactory->SetDoc(IndexedDoc);
}

void TIndexedDocStorageBase::CommitDoc() {
    InvCreator->MakePortion();
    IndexedDoc->SetArchive(OutArchive.Buffer().Data(), OutArchive.Buffer().Size());
    OutArchive.Buffer().Clear();
}

void TIndexedDocStorage::AddInvCreator(NIndexerCore::IDirectTextCallback4* callback) {
    Y_VERIFY(!!PrewalrusProcessor, "Incorrect PrewalrusProcessor");
    PrewalrusProcessor->AddInvCreator(callback);
}

void TIndexedDocStorage::AddDTCallback(NIndexerCore::IDirectTextCallback2* callback) {
    Y_VERIFY(!!PrewalrusProcessor, "Incorrect PrewalrusProcessor");
    PrewalrusProcessor->AddDirectTextCallback(callback);
}

TIndexedDocStorage::TIndexedDocStorage(TIndexedDocStorageConstructionData& data, const TSimpleSharedPtr<TCompatiblePureContainer>* pure)
    : TIndexedDocStorageBase(data.AdditionalStorages, data.Cfg.UseExtProcs, data.Cfg.StopWordFile, data.Cfg.StripKeysOnIndex)
    , AttrPr(&data.AttrCfg, data.Filter, data.GroupAttributer)
    , GrpAction(&data.Cfg)
    , GroupingConfig(nullptr)
{
    TGroupProcessorConfig* grpCfg = !!data.Cfg.Indexaa ? (TGroupProcessorConfig*)&data.Cfg : nullptr;

    PrewalrusProcessor.Reset(new TPrewalrusProcessor(
        !!data.CustomParsedDocStorage ? data.CustomParsedDocStorage : new TParsedDocStorage(data.Cfg),
        &data.Cfg,
        GetDisamber(),
        GetInvCreator(),
        grpCfg,
        data.EventsProcessor,
        pure)
    );

    if (data.Cfg.UseFullArchive) {
        if (data.Cfg.IsRtYServer) {
            FullArchiveFlusher.Reset(new TFullArchiveFlusher(&data.Cfg));
        } else {
            OutFullArchive.Reset(new TBufferOutput(1u << 16));
            FullArchiveFlusher.Reset(new TFullArchiveFlusher(OutFullArchive.Get(), /*writeHeader=*/false));
        }
    }
    if (!data.Cfg.IsRtYServer) {
        TextPr.Reset(new TTextFeaturesProcessor(&data.Cfg, data.ShopModel, data.ShopWordHolder));
        PrewalrusProcessor->AddAction(TextPr.Get());
    }

    if (data.Cfg.UseSentenceLengthProcessor) {
        PrewalrusProcessor->AddDirectTextCallback(&SentenceLengthProcessor);
    }

    if (!!data.Cfg.TQueryFactorsConfig::GetConfigDir()) { // calc query factors if we have config dir
        QueryBasedProcessor.Reset(new TQueryBasedProcessor(data.Cfg));
        PrewalrusProcessor->AddDirectTextCallback(QueryBasedProcessor.Get());
    }

    if (!!data.Cfg.TTitleFeaturesConfig::PureTrieFile) {
        TitleFeaturesProcessor.Reset(new TTitleFeaturesProcessor(data.Cfg));
        PrewalrusProcessor->AddDirectTextCallback(TitleFeaturesProcessor.Get());
    }

    PrewalrusProcessor->SetArcCreator(&data.Cfg, GetArchiveStream());
    PrewalrusProcessor->AddAction(&AttrPr);
    if (!grpCfg)
        PrewalrusProcessor->AddAction(&GrpAction);
}

TIndexedDocStorage::~TIndexedDocStorage() {
}

void TIndexedDocStorage::InitAdultDetector(const TString& adultFileName, TAdultPreparat& adultPreparat, bool usePrepared) {
    // adult detector working together QueryBasedProcessor
    if (!!QueryBasedProcessor)
        QueryBasedProcessor->InitAdultDetector(adultFileName, adultPreparat, usePrepared);
}

void TIndexedDocStorage::SetParserConf(const TString& name) {
    PrewalrusProcessor->SetParserConf(name);
}

void TIndexedDocStorage::IndexDoc(const TDocInfoEx& docInfo, TFullDocAttrs& extAttrs,
                                  NRealTime::TIndexedDoc* indexedDoc,
                                  const TString& url4Archive, const bool httpCutterUsage,
                                  const NIndexerCore::TExtraTextZones* extraTextZones)
{
    AddDoc(indexedDoc);
    char* url = (char*)docInfo.DocHeader->Url;
    if (GetHttpPrefixSize(url) == 7 && httpCutterUsage)
        memmove(url, url + 7, strlen(url + 7) + 1);

    // hack for properly working numerator
    TString urlWithPrefix = url;
    if (GetHttpPrefixSize(url) == 0) {
        urlWithPrefix = TString("http://") + urlWithPrefix;
    }
    extAttrs.AddAttr(PP_BASE, urlWithPrefix, TFullDocAttrs::AttrAuxPars);

    indexedDoc->SetUrl(url);
    PrewalrusProcessor->SetCurrentUrl(url4Archive);
    PrewalrusProcessor->ProcessOneDoc(&docInfo, &extAttrs, extraTextZones);

    CommitDoc();

    if (!!FullArchiveFlusher)
        FullArchiveFlusher->FlushDoc(&docInfo, extAttrs);
    FillAttributes(GroupingConfig, extAttrs, *indexedDoc);
    if (!!OutFullArchive) {
        indexedDoc->SetFullArchive(OutFullArchive->Buffer().Data(), OutFullArchive->Buffer().Size());
        OutFullArchive->Buffer().Clear();
    }
    size_t maxSentencesNumber = 0;
    const TSentenceLensProcessor::TSentenceLengths& sentencesLength = SentenceLengthProcessor.GetSentencesInfo(maxSentencesNumber);
    if (maxSentencesNumber != 0)
        indexedDoc->SetSentencesLength(&sentencesLength[0], maxSentencesNumber);
}

void TIndexedDocStorage::FillAttributesToExtAttrs(TFullDocAttrs& extAttrs, const NRealTime::TIndexedDoc& indexedDoc) {
    for (size_t i = 0; i < indexedDoc.AttrsSize(); ++i) {
        const ::NRealTime::TIndexedDoc_TAttr& attr = indexedDoc.GetAttrs(i);

        if (attr.GetSizeOfInt() == NRealTime::TIndexedDoc::TAttr::BAD)
            extAttrs.AddAttr(attr.GetName(), attr.GetValue(), attr.GetType());
        else if (attr.GetSizeOfInt() == NRealTime::TIndexedDoc::TAttr::I16)
            extAttrs.AddAttr(attr.GetName(), attr.GetValue(), attr.GetType(), NGroupingAttrs::TConfig::I16);
        else if (attr.GetSizeOfInt() == NRealTime::TIndexedDoc::TAttr::I32)
            extAttrs.AddAttr(attr.GetName(), attr.GetValue(), attr.GetType(), NGroupingAttrs::TConfig::I32);
        else if (attr.GetSizeOfInt() == NRealTime::TIndexedDoc::TAttr::I64)
            extAttrs.AddAttr(attr.GetName(), attr.GetValue(), attr.GetType(), NGroupingAttrs::TConfig::I64);
        else
            Y_FAIL("Incorrect group attribute type");
    }
}

void TIndexedDocStorage::FillAttributes(const NGroupingAttrs::TConfig* groupingConfig, const TFullDocAttrs& extAttrs, NRealTime::TIndexedDoc& indexedDoc) {
    for (TFullDocAttrs::TConstIterator i = extAttrs.Begin(); i != extAttrs.End(); ++i) {
        const TFullDocAttrs::TAttr& a = *i;
        // skip attributes which type more than TFullDocAttrs::AttrErf
        // all attrubutes combinations shall pass
        if (a.Type > ((ui32)TFullDocAttrs::AttrErf << 1) - 1)
            continue;

        NRealTime::TIndexedDoc_TAttr &attr = *indexedDoc.AddAttrs();
        attr.SetName(a.Name);
        attr.SetValue(a.Value);
        attr.SetType(a.Type);
        bool groupingAttr = a.Type & (TFullDocAttrs::AttrGrName | TFullDocAttrs::AttrGrInt);
        if (groupingAttr) {
            bool hasConfig = !!groupingConfig;
            ui32 attrNum = (hasConfig) ? groupingConfig->AttrNum(a.Name.data()) : NGroupingAttrs::TConfig::NotFound;
            bool inConfig = attrNum != NGroupingAttrs::TConfig::NotFound;
            if (!hasConfig || inConfig) {
                NRealTime::TIndexedDoc::TAttr::TAttrType attrType;
                NGroupingAttrs::TConfig::Type configType = (inConfig) ? groupingConfig->AttrType(attrNum) : a.SizeOfInt;
                switch (configType) {
                    case NGroupingAttrs::TConfig::I16:
                        attrType = NRealTime::TIndexedDoc::TAttr::I16;
                        break;
                    case NGroupingAttrs::TConfig::I32:
                        attrType = NRealTime::TIndexedDoc::TAttr::I32;
                        break;
                    case NGroupingAttrs::TConfig::I64:
                        attrType = NRealTime::TIndexedDoc::TAttr::I64;
                        break;
                    case NGroupingAttrs::TConfig::BAD:
                    default:
                        attrType = NRealTime::TIndexedDoc::TAttr::BAD;
                        break;
                }
                if (attrType != NRealTime::TIndexedDoc::TAttr::BAD)
                    attr.SetSizeOfInt(attrType);
            }
        }
    }
}

void TIndexedDocStorage::ClearAndFlushArchiveBuffers() {
    GetArchiveStream().Buffer().Clear();
    if (!!OutFullArchive)
        OutFullArchive->Buffer().Clear();
}

static inline
void InitLemmerOptions(NLemmer::TAnalyzeWordOpt& opt, bool useLemmer) {
    opt = NLemmer::TAnalyzeWordOpt::IndexerOpt();

    if (!useLemmer) {
        // Disable lemmer
        opt.AcceptDictionary.Reset();
        opt.AcceptSob.Reset();
        opt.AcceptBastard.Reset();
        opt.AcceptFoundling = ~TLangMask();
    }
}

TIndexedDocDirectIndexStorage::TIndexedDocDirectIndexStorage(bool useLemmer)
    : TIndexedDocStorageBase(nullptr, false, TString())
    , PlainArchiveCreator(new TArchiveCreator(GetArchiveStream(), TArchiveCreator::SaveDocHeader))
    , FreqCalculator(new TFreqCalculator(false))
{
    NLemmer::TAnalyzeWordOpt lemmerOptions[1];
    InitLemmerOptions(lemmerOptions[0], useLemmer);
    NIndexerCore::TDTCreatorConfig dtcCfg;
    // TODO: Load pure. file to fix the next comment!!!
    DirectCreator.Reset(new NIndexerCore::TDirectTextCreator(dtcCfg, TLangMask(LI_BASIC_LANGUAGES), LANG_UNK, lemmerOptions, 1));
    DirectIndex.Reset(new NIndexerCore::TDirectIndex(*DirectCreator));
    DirectIndex->SetArcCreator(PlainArchiveCreator.Get());
    DirectIndex->SetInvCreator(GetInvCreator());
    // this is required because basesearch determines DocumentCount based on fake key YDX_MAX_FREQ_KEY (seems like an arctic fox)
    DirectIndex->AddDirectTextCallback(FreqCalculator.Get());
}

TIndexedDocDirectIndexStorage::~TIndexedDocDirectIndexStorage() {
}

NIndexerCore::TDirectIndex& TIndexedDocDirectIndexStorage::GetDirectIndex() {
    return *DirectIndex;
}
