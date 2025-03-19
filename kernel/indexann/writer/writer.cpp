/// author@ vvp@ Victor Ploshikhin
/// created: Sep 27, 2013 5:06:40 PM
/// see: ROBOT-2993

#include "writer.h"

#include <dict/recognize/queryrec/queryrecognizer.h>

#include <kernel/indexer/posindex/invcreator.h>
#include <kernel/sent_lens/sent_lens.h>
#include <kernel/walrus/advmerger.h>
#include <kernel/indexann/interface/names.h>

#include <ysite/directtext/freqs/freqs.h>
#include <ysite/directtext/sentences_lens/sentenceslensproc.h>
#include <ysite/yandex/pure/pure_container.h>

#include <kernel/keyinv/indexfile/indexstorageface.h>
#include <kernel/tarc/iface/tarcio.h>

#include <util/folder/tempdir.h>
#include <util/generic/ptr.h>
#include <util/folder/dirut.h>

namespace NIndexAnn {

class TDataCache {
public:
    void Add(size_t region, size_t dataKey, const TArrayRef<const char>& data);
    void Flush(size_t firstBreakId, size_t lastBreakId, IDocDataWriter&);

private:
    struct TData {
        size_t  Region;
        size_t  DataKey;
        TBuffer Data;

        TData(size_t region=0, size_t dataKey=0)
            : Region(region)
            , DataKey(dataKey)
        {
        }
    };

    TVector<TData> Data;
};



void TDataCache::Add(size_t region, size_t dataKey, const TArrayRef<const char>& data)
{
    TData saveData(region, dataKey);
    saveData.Data.Assign(data.data(), data.size());

    Data.push_back(saveData);
}

void TDataCache::Flush(size_t firstBreakId, size_t lastBreakId, IDocDataWriter& writer)
{
    const size_t totalData = Data.size();
    if (0 == totalData) {
        return;
    }

    for (size_t breakId = firstBreakId; breakId < lastBreakId; ++breakId) {
        for (size_t i = 0; i < totalData; ++i) {
            const TData& saveData = Data[i];
            TArrayRef<const char> data(saveData.Data.Data(), saveData.Data.Size());
            writer.Add(breakId, saveData.Region, saveData.DataKey, data);
        }
    }

    Data.clear();
}

class TWriterImplBase {
public:
    TWriterImplBase(const TString& indexPath, const TString& tmpDir, const TWriterConfig& cfg, TAtomicSharedPtr<IDocDataWriter> dataWriter);
    TWriterImplBase(TSimpleSharedPtr<IYndexStorageFactory> storage, TAtomicSharedPtr<IDocDataWriter> dataWriter, const TWriterConfig& cfg);
    virtual ~TWriterImplBase();

    void AddDirectTextCallback(NIndexerCore::IDirectTextCallback2* obj);
    void AddDirectTextCallback(NIndexerCore::IDirectTextCallback5* obj);

    void SetAutoIncrementBreaks(bool);

    virtual void StartDoc(ui32 docId, ui64 feedId);
    void FinishDoc(bool makePortion, TFullDocAttrs* docAttrs);

    bool StartSentence(const TUtf16String&, const TSentenceParams& params, const void* callbackV5Data);
    bool StartSentence(const TString&, const TSentenceParams& params, const void* callbackV5Data);
    void SetWarningStream(IOutputStream& newCerr);

    void AddData(size_t region, size_t dataKey, const TArrayRef<const char>& data);

    TSimpleSharedPtr<IYndexStorageFactory> GetStorageFactory() const
    {
        return Storage;
    }
    void MakePortion()
    {
        Ic.MakePortion();
    }

protected:
    void InitIndex(NIndexerCore::TDirectIndex* index);
    void WriteSentenceData();

protected:
    typedef TSimpleSharedPtr<NQueryRecognizer::TFactorMill> TFactorMillPtr;

    THolder<TTempDir>             TmpDir;

    TSimpleSharedPtr<IYndexStorageFactory>  Storage;
    NIndexerCore::TIndexStorageFactory*     RealFsStorage;
    NIndexerCore::TDirectTextCreator        Dtc;
    NIndexerCore::TInvCreatorConfig         InvCfg;
    NIndexerCore::TInvCreatorDTCallback     Ic;
    TFreqCalculator                         FreqCalculator;
    NIndexerCore::TDirectIndex*             Index;

    TFullArchiveDocHeader                   CurDocHeader;
    TDocInfoEx                              CurDocInfo;
    bool                                    CanAddData;
    size_t                                  CurBreak;
    size_t                                  TheSameShitBreaksNum;
    ui32                                    CurDocId;
    TAtomicSharedPtr<IDocDataWriter>       DataWriter;

    TBlob                               RecognizerData;
    THolder<TQueryRecognizer>           Recognizer;
    TFactorMillPtr                      FactorMill;

    TDataCache                          DataCache;
    bool                                AutoIncrementBreaks;
    IOutputStream*                      LocalCerr;
};


TWriterImplBase::TWriterImplBase(TSimpleSharedPtr<IYndexStorageFactory> storage, TAtomicSharedPtr<IDocDataWriter> dataWriter,
                                 const TWriterConfig& cfg)
    : Storage(storage)
    , RealFsStorage(dynamic_cast<NIndexerCore::TIndexStorageFactory*>(Storage.Get()))
    , Dtc(cfg.DtcCfg, nullptr)
    , InvCfg(cfg.InvIndexDocCount, cfg.InvIndexMem)
    , Ic(Storage.Get(), InvCfg)
    , FreqCalculator(false)
    , Index(nullptr)
    , CanAddData(false)
    , CurBreak(FIRST_ANN_BREAK_ID)
    , TheSameShitBreaksNum(0)
    , CurDocId(0)
    , DataWriter(dataWriter)
    , AutoIncrementBreaks(cfg.AutoIncrementBreaks)
    , LocalCerr(&Cerr)
{
    if (cfg.RecognizerDataPath.IsDefined()) {
        TFileInput weightsInput(cfg.RecognizerDataPath / "queryrec.weights");
        TFileInput filtersInput(cfg.RecognizerDataPath / "queryrec.filters");
        RecognizerData = TBlob::FromFileSingleThreaded(cfg.RecognizerDataPath / "queryrec.dict");

        FactorMill.Reset(new NQueryRecognizer::TFactorMill(RecognizerData));
        Recognizer.Reset(new TQueryRecognizer(FactorMill, weightsInput, TGeoTreePtr(new TGeoTree()), filtersInput));
    }

    CurDocHeader.Url[0] = 0;
    CurDocInfo.DocHeader = &CurDocHeader;
    CurDocHeader.Encoding = CODES_UTF8;
}

TWriterImplBase::TWriterImplBase(const TString& indexPath, const TString& tmpDir, const TWriterConfig& cfg, TAtomicSharedPtr<IDocDataWriter> dataWriter)
    : TWriterImplBase(new NIndexerCore::TIndexStorageFactory(), dataWriter, cfg)
{
    Y_ASSERT(tmpDir && indexPath);
    if (!TmpDir.Get()) {
        Y_ASSERT(!tmpDir.empty());
        TmpDir.Reset(new TTempDir(TTempDir::NewTempDir(tmpDir)));
    }

    RealFsStorage->InitIndexResources(indexPath.data(), TmpDir->Name().data(), nullptr);
}

void TWriterImplBase::InitIndex(NIndexerCore::TDirectIndex* index)
{
    Index = index;
    Index->SetInvCreator(&Ic);
    Index->AddDirectTextCallback(&FreqCalculator);

    SetAutoIncrementBreaks(AutoIncrementBreaks);
}

void TWriterImplBase::SetAutoIncrementBreaks(bool f)
{
    Index->SetIgnoreStoreTextBreaks(!f);
    Index->SetNoImplicitBreaks(!f);
}

void TWriterImplBase::AddDirectTextCallback(NIndexerCore::IDirectTextCallback2* obj)
{
    Index->AddDirectTextCallback(obj);
}

void TWriterImplBase::AddDirectTextCallback(NIndexerCore::IDirectTextCallback5* obj)
{
    Index->AddDirectTextCallback(obj);
}

TWriterImplBase::~TWriterImplBase()
{
    if (nullptr != RealFsStorage) {
        Ic.MakePortion();
        MergePortions(*RealFsStorage, YNDEX_VERSION_FINAL_DEFAULT, true, true);
    }
}

void TWriterImplBase::SetWarningStream(IOutputStream& newCerr) {
    LocalCerr = &newCerr;
}

void TWriterImplBase::StartDoc(ui32 docId, ui64 feedId)
{
    Y_VERIFY(docId <= DOC_LEVEL_Max, "maximum doc id achieved: DOC_LEVEL_Max=%d", DOC_LEVEL_Max);
    Y_VERIFY(CurDocId == 0 || CurDocId < docId, "bad doc id order");

    DataWriter->StartDoc(docId);

    CurDocInfo.FeedId = feedId;
    CurDocId = docId;
    CurBreak = FIRST_ANN_BREAK_ID;
    TheSameShitBreaksNum = 0;
    Index->AddDoc(docId, LANG_UNK);
}

bool TWriterImplBase::StartSentence(const TUtf16String& wSent, const TSentenceParams& sentParams, const void* callbackV5Data)
{
    if (sentParams.IncBreak) {
        WriteSentenceData();
    }

    if (CurBreak == BREAK_LEVEL_Max) {
        *LocalCerr << "too many sentences for one document: " << CurDocId << Endl;
        CanAddData = false;
        return false;
    }

    CanAddData = true;
    TLangMask langMask = sentParams.LangMask.Empty() ? NLanguageMasks::LemmasInIndex() : sentParams.LangMask;
    ELanguage lang = LANG_UNK;

    // Recognition can work slowly.
    if (sentParams.TextLanguage.Defined()) {
        lang = sentParams.TextLanguage.GetRef();
    } else {
        Y_VERIFY(Recognizer, "no language data providen but language recognizer is not initialized");
        lang = Recognizer->RecognizeParsedQueryLanguage(wSent).GetMainLang();
    }

    Index->StoreText(wSent.data(), wSent.size(), sentParams.RLevel, langMask, lang, 0, sentParams.Attrs, nullptr, callbackV5Data);

    TheSameShitBreaksNum = 0;
    const ui16 breakNum = Index->GetPosition().Break();
    if (breakNum > CurBreak) {
        TheSameShitBreaksNum = breakNum - CurBreak;
        *LocalCerr << CurDocId << ", bad sentence: '" << wSent << "' breaks mismatch: " << CurBreak << " < " << breakNum << "\n";
        CurBreak = breakNum;
    }

    if (CurBreak != breakNum) {
        ythrow yexception() << "bad break: " << CurBreak << " != " << breakNum << "\n";
    }

    if (sentParams.IncBreak) {
        ++CurBreak;
        Index->IncBreak();
    }

    return true;
}

bool TWriterImplBase::StartSentence(const TString& sent, const TSentenceParams& sentParams, const void* callbackV5Data)
{
    try {
        const TUtf16String wSent = UTF8ToWide(sent);
        return StartSentence(wSent, sentParams, callbackV5Data);
    } catch (const yexception& e) {
        *LocalCerr << e.what() << Endl;
        CanAddData = false;
        return false;
    }
}

void TWriterImplBase::FinishDoc(bool makePortion, TFullDocAttrs* docAttrs)
{
    WriteSentenceData();
    DataWriter->FinishDoc();

    Index->CommitDoc(&CurDocInfo, docAttrs);
    if (!RealFsStorage && makePortion) {
        Ic.MakePortion();
    }
}

void TWriterImplBase::AddData(size_t region, size_t dataKey, const TArrayRef<const char>& data)
{
    if (false == CanAddData) {
        return;
    }

    DataCache.Add(region, dataKey, data);
}

void TWriterImplBase::WriteSentenceData()
{
    DataCache.Flush(CurBreak - 1 - TheSameShitBreaksNum, CurBreak, *DataWriter);
}


class TWriterImpl : public TWriterImplBase {
public:
    TWriterImpl(const TString& indexPath, const TString& tmpDir, const TWriterConfig& cfg, TAtomicSharedPtr<IDocDataWriter> dataWriter)
        : TWriterImplBase(indexPath, tmpDir, cfg, dataWriter)
        , IndexWithoutArc(Dtc)
    {
        InitIndex(&IndexWithoutArc);
    }

    TWriterImpl(TSimpleSharedPtr<IYndexStorageFactory> storage, TAtomicSharedPtr<IDocDataWriter> dataWriter, const TWriterConfig& cfg)
        : TWriterImplBase(storage, dataWriter, cfg)
        , IndexWithoutArc(Dtc)
    {
        InitIndex(&IndexWithoutArc);
    }

private:
    NIndexerCore::TDirectIndex IndexWithoutArc;
};


class TWriterWithArcImpl : public TWriterImplBase {
public:
    TWriterWithArcImpl(const TString& indexPath, const TString& tmpDir, const TWriterConfig& cfg, TAtomicSharedPtr<IDocDataWriter> dataWriter)
        : TWriterImplBase(indexPath, tmpDir, cfg, dataWriter)
        , ArchiveArcFileName(indexPath + INDEX_ANN_SUFFIX_ARC_FILE)
        , ArchiveDirFileName(indexPath + INDEX_ANN_SUFFIX_DIR_FILE)
    {
        IndexWithArcHolder.Reset(new NIndexerCore::TDirectIndexWithArchive(Dtc, ArchiveArcFileName));
        InitIndex(IndexWithArcHolder.Get());
    }

    TWriterWithArcImpl(TSimpleSharedPtr<IYndexStorageFactory> storage, TAtomicSharedPtr<IDocDataWriter> dataWriter, const TWriterConfig& cfg, const TArchiveWriterConfig& archiveCfg)
        : TWriterImplBase(storage, dataWriter, cfg)
    {
        IndexWithArcHolder.Reset(new NIndexerCore::TDirectIndexWithArchive(Dtc, archiveCfg.ArchiveStream, archiveCfg.UseHeader, archiveCfg.ArchiveVersion));
        InitIndex(IndexWithArcHolder.Get());
    }

    ~TWriterWithArcImpl() override
    {
        IndexWithArcHolder.Destroy(); // I want to make sure that arc file is written

        if (!!ArchiveArcFileName && !!ArchiveDirFileName)
            MakeArchiveDir(ArchiveArcFileName, ArchiveDirFileName);
    }

private:
    TString ArchiveArcFileName;
    TString ArchiveDirFileName;

    THolder<NIndexerCore::TDirectIndexWithArchive> IndexWithArcHolder;
};

class TAnnDataMemoryWriter : public IDocDataWriter {
private:
    TPortionPB& Portion;

public:
    TAnnDataMemoryWriter(TPortionPB& portion)
        : Portion(portion)
    {
    }

    void Add(ui32 breakId, ui32 regionId, ui32 typeId, TArrayRef<const char> data) override
    {
        auto& dataPortion = *Portion.MutableRow()->AddElements();
        dataPortion.SetElementId(breakId);
        dataPortion.SetEntryKey(regionId);
        dataPortion.SetItemKey(typeId);
        dataPortion.SetData(data.data(), data.size());
    }

    void StartDoc(ui32) override {}
    void FinishDoc() override {}
};

class TMemoryWriterImplBase : public TWriterImplBase {
public:
    TMemoryWriterImplBase(const TWriterConfig& cfg, IYndexStorage::FORMAT format)
        : TWriterImplBase(
              MakeSimpleShared<NIndexerCore::TMemoryPortionFactory>(format),
              MakeAtomicShared<TAnnDataMemoryWriter>(Portion),
              cfg
          )
    {}

    virtual void StartPortion()
    {
        Portion.Clear();
        NIndexerCore::TMemoryPortionFactory* storage = static_cast<NIndexerCore::TMemoryPortionFactory*>(Storage.Get());
        storage->ClearPortions();
        CurDocId = 0;
    }

    void StartDoc(ui32 docId, ui64 feedId) override
    {
        Y_VERIFY(docId <= DOC_LEVEL_Max, "maximum doc id achieved: DOC_LEVEL_Max");

        CurDocInfo.FeedId = feedId;
        CurDocId = docId;
        CurBreak = 1;
        TheSameShitBreaksNum = 0;
        Index->AddDoc(docId, LANG_UNK);
    }

    virtual TPortionPB& DonePortion()
    {
        Ic.MakePortion();

        // write portions
        NIndexerCore::TMemoryPortionFactory* storage = static_cast<NIndexerCore::TMemoryPortionFactory*>(Storage.Get());
        const NIndexerCore::TMemoryPortionFactory::TPortions& portions = storage->GetPortions();
        TBuffer InvPortionsBuffer;
        NIndexerCore::CopyMemoryPortions(InvPortionsBuffer, portions);
        Portion.SetInv(InvPortionsBuffer.data(), InvPortionsBuffer.size());

        return Portion;
    }

private:
    TPortionPB Portion;
};


class TMemoryWriterImpl : public TMemoryWriterImplBase {
public:
    TMemoryWriterImpl(const TWriterConfig& cfg, IYndexStorage::FORMAT format)
        : TMemoryWriterImplBase(cfg, format)
        , IndexWithoutArc(Dtc)
    {
        InitIndex(&IndexWithoutArc);
    }

private:
    NIndexerCore::TDirectIndex IndexWithoutArc;
};

class TMemoryWriterWithArcImpl : public TMemoryWriterImplBase {
public:
    TMemoryWriterWithArcImpl(const TWriterConfig& cfg, IYndexStorage::FORMAT format)
        : TMemoryWriterImplBase(cfg, format)
        , ArchiveConfig(&ArchiveStream, false, ARC_COMPRESSED_EXT_INFO)
        , IndexWithArc(Dtc, ArchiveConfig.ArchiveStream, ArchiveConfig.UseHeader, ArchiveConfig.ArchiveVersion)
    {
        InitIndex(&IndexWithArc);
    }

    TPortionPB& DonePortion() override
    {
        TPortionPB& portion = TMemoryWriterImplBase::DonePortion();

        TBuffer& archiveBuffer = ArchiveStream.Buffer();
        portion.SetArc(archiveBuffer.data(), archiveBuffer.size());
        archiveBuffer.Clear();

        return portion;
    }

    TBufferOutput& GetArc()
    {
        return ArchiveStream;
    }

private:
    TBufferOutput ArchiveStream;
    TArchiveWriterConfig ArchiveConfig;

    NIndexerCore::TDirectIndexWithArchive IndexWithArc;
};

TWriter::TWriter()
{
}

TWriter::TWriter(const TString& indexPath, const TString& tmpDir, const TWriterConfig& cfg, TAtomicSharedPtr<IDocDataWriter> dataWriter, bool needArc)
{
    Y_ENSURE(dataWriter, "dataWriter should not be a nullptr");
    if (needArc) {
        Impl.Reset(new TWriterWithArcImpl(indexPath, tmpDir, cfg, dataWriter));
    } else {
        Impl.Reset(new TWriterImpl(indexPath, tmpDir, cfg, dataWriter));
    }
}

TWriter::TWriter(TSimpleSharedPtr<IYndexStorageFactory> storage, TAtomicSharedPtr<IDocDataWriter> dataWriter, const TWriterConfig& cfg, const TArchiveWriterConfig& archiveCfg)
{
    if (archiveCfg.ArchiveStream) {
        Impl.Reset(new TWriterWithArcImpl(storage, dataWriter, cfg, archiveCfg));
    } else {
        Impl.Reset(new TWriterImpl(storage, dataWriter, cfg));
    }
}

TWriter::~TWriter()
{
}

void TWriter::AddDirectTextCallback(NIndexerCore::IDirectTextCallback2* obj)
{
    Impl->AddDirectTextCallback(obj);
}

void TWriter::AddDirectTextCallback(NIndexerCore::IDirectTextCallback5* obj)
{
    Impl->AddDirectTextCallback(obj);
}

void TWriter::StartDoc(ui32 docId, ui64 feedId)
{
    Impl->StartDoc(docId, feedId);
}

void TWriter::FinishDoc(bool makePortion, TFullDocAttrs* docAttrs)
{
    Impl->FinishDoc(makePortion, docAttrs);
}

bool TWriter::StartSentence(const TUtf16String& sent, const TSentenceParams& sentParams, const void* callbackV5Data)
{
    return Impl->StartSentence(sent, sentParams, callbackV5Data);
}

bool TWriter::StartSentence(const TString& sent, const TSentenceParams& sentParams, const void* callbackV5Data)
{
    return Impl->StartSentence(sent, sentParams, callbackV5Data);
}

void TWriter::AddData(size_t region, size_t dataKey, const TArrayRef<const char>& data)
{
    Impl->AddData(region, dataKey, data);
}

void TWriter::SetWarningStream(IOutputStream& newCerr)
{
    Impl->SetWarningStream(newCerr);
}

void TWriter::SetAutoIncrementBreaks(bool f)
{
    Impl->SetAutoIncrementBreaks(f);
}

TMemoryWriter::TMemoryWriter(const TWriterConfig& cfg, IYndexStorage::FORMAT format, bool needArc)
{
    if (needArc) {
        Impl.Reset(new TMemoryWriterWithArcImpl(cfg, format));
    } else {
        Impl.Reset(new TMemoryWriterImpl(cfg, format));
    }
}

void TMemoryWriter::StartPortion()
{
    TMemoryWriterImplBase* memImpl = static_cast<TMemoryWriterImplBase*>(Impl.Get());
    memImpl->StartPortion();
}

TPortionPB& TMemoryWriter::DonePortion()
{
    TMemoryWriterImplBase* memImpl = static_cast<TMemoryWriterImplBase*>(Impl.Get());
    return memImpl->DonePortion();
}


TFullAccessMemoryWriter::TFullAccessMemoryWriter(const TWriterConfig& cfg, IYndexStorage::FORMAT format, bool needArc): TMemoryWriter(cfg, format, needArc), NeedArc(needArc) {}


NIndexerCore::TMemoryPortionFactory& TFullAccessMemoryWriter::GetStorageFactory() const
{
    NIndexerCore::TMemoryPortionFactory *storage = static_cast<NIndexerCore::TMemoryPortionFactory*>(Impl->GetStorageFactory().Get());
    return *storage;
}

TBuffer& TFullAccessMemoryWriter::GetArchBuffer() const
{
    if (!NeedArc) {
        ythrow yexception() << "Archive file isn't supported" << "\n";
    }
    TMemoryWriterWithArcImpl* memImpl = static_cast<TMemoryWriterWithArcImpl*>(Impl.Get());

    return memImpl->GetArc().Buffer();
}

void TFullAccessMemoryWriter::MakePortion()
{
    Impl.Get()->MakePortion();
}

} // namespace NIndexAnn
