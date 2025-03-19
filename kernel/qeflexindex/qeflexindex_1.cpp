#include <util/ysaveload.h>

#include <util/generic/buffer.h>
#include <util/folder/dirut.h>

#include <library/cpp/packedtypes/packed.h>

#include <kernel/keyinv/indexfile/oldindexfile.h>
#include <kernel/index_mapping/index_mapping.h>
#include <kernel/searchlog/errorlog.h>

#include "qeflexindex_1.h"
#include "qeflexindex_1_impl.h"

TQEFlexIndexEntryCodec::TQEFlexIndexEntryCodec(const TQEFlexIndexMetaData& metadata)
{
    ui64 sizeWidth = 0;
    switch(metadata.IndexType) {
        case UIT_WEB:
        case UIT_IMG:
            sizeWidth = metadata.IndexVersion >= 3 ? 24 : 28;
            break;
        case UIT_VIDEO:
            sizeWidth = metadata.IndexVersion >= 4 ? 24 : 28;
            break;
    }
    SizeMask = (1LL << sizeWidth) - 1;
    NonSizeShift = sizeWidth + SIZE_SHIFT;
    NonSizeMask = (1LL << (64 - 2 - SIZE_SHIFT - sizeWidth)) - 1;
}

class TImplHits : public TQEFlexIndex4Indexing::IImpl
{
private:
    NIndexerCore::TRawIndexFile     IndexFile;
    TQEFlexIndexEntryCodec          EntryCodec;

public:
    TImplHits(const TQEFlexIndexMetaData& metadata,
              TString dstIndPrefix)
      : IImpl(metadata, dstIndPrefix)
      , EntryCodec(Metadata)
    {
        IndexFile.Open(dstIndPrefix.data());
    }

    void FinalizeImpl() override {
        Finalize();
        IndexFile.CloseEx();
    }

    ~TImplHits() override {
       FinalizeImpl();
    }

    void SaveCurKeyImpl() override
    {
        TVector<SUPERLONG> curPositions(2 + CurDocsData.size());

        ui64 offset = CurDatOffset;

        // per-query data
        {
            ui32 sizeIn4 = SaveDataPadded(CurKeyData);
            TQEFlexIndexQEntry qe = { offset+1, sizeIn4 };
            curPositions[1] = EntryCodec.Code(qe);
        }
        // per-doc data
        for (size_t i = 0; i < CurDocsData.size(); ++i) {
    #ifndef NDEBUG
            if (i && (CurDocsData[i-1].Id == CurDocsData[i].Id)) {
                ythrow yexception() << "Duplicate docId[" <<  CurDocsData[i].Id << "] for query [" <<  CurKeyRaw.data() << "]";
            }
    #endif
            ui32 sizeIn4 = SaveDataPadded(CurDocsData[i].Data);
            TQEFlexIndexDEntry de = { CurDocsData[i].Id, sizeIn4 };
            curPositions[i+2] = EntryCodec.Code(de);
        }
        // all per-query data
        {
            ui32 sizeIn4 = ui32(CurDatOffset - offset);
            TQEFlexIndexQEntry qe = { offset, sizeIn4 };
            curPositions[0] = EntryCodec.Code(qe);
        }
        IndexFile.StorePositions((CurKeyRaw).data(), &curPositions.front(), curPositions.size());
    }
};



class TImplInvRaw : public TImplInvRawBase
{
private:
    void WriteElId(i32 elId, i64& len)
    {
        char buf[4];
        size_t l = out_long(elId, buf);
        IndexFile.WriteInvData(buf, l);
        len += i64(l);
    }
public:
    TImplInvRaw(const TQEFlexIndexMetaData& metadata,
                TString dstIndPrefix)
        : TImplInvRawBase(metadata, dstIndPrefix) {}
    void SaveCurKeyImpl() override
    {
        i64 len = 0;
        i64 nEls = i64(CurDocsData.size());

        // per-query data
        WriteInv(CurKeyData, len);

        i32 prevElId = 0;
        // per-doc data
        for (i64 i = 0; i < nEls; ++i) {
    #ifndef NDEBUG
            if (i && (CurDocsData[i-1].Id == CurDocsData[i].Id)) {
                ythrow yexception() << "Duplicate docId[" <<  CurDocsData[i].Id << "] for query [" <<  CurKeyRaw.data() << "]";
            }
    #endif
            i32 elId = (CurDocsData[i].Id);
            WriteElId(elId - prevElId, len);
            prevElId = elId;
            WriteInv(CurDocsData[i].Data, len);
        }
        int dataLen =   IndexFile.GetKeyDataLen(nEls)
                      + IndexFile.GetKeyDataLen(len);

        IndexFile.WriteKey((CurKeyRaw).data(), dataLen);

        IndexFile.WriteKeyData(nEls);
        IndexFile.WriteKeyData(len);
    }

    ~TImplInvRaw() override {
       FinalizeImpl();
    }
};

void TQEFlexIndex4Indexing::Init(const TQEFlexIndexMetaData& metadata, TString dstIndPrefix)
{
    switch (metadata.IndexType) {
        case UIT_WEB:
            if (metadata.IndexVersion == 0) { // special case for diversity_indexer
                Impl.Reset(new TImplHits(metadata, dstIndPrefix));
                return;
            }
            if (metadata.IndexVersion >= 10) {
                CreateIndexSpecificImpl(metadata, dstIndPrefix.data());
                return;
            }
            ythrow yexception() << "index version " << (ui32)metadata.IndexVersion << " is not supported";
            break;
        case UIT_IMG:
            if (metadata.IndexVersion <= 5) {
                Impl.Reset(new TImplHits(metadata, dstIndPrefix));
            } else {
                CreateIndexSpecificImpl(metadata, dstIndPrefix.data());
            }
            break;
        case UIT_VIDEO:
            if (metadata.IndexVersion <= 5) {
                Impl.Reset(new TImplHits(metadata, dstIndPrefix));
            } else {
                CreateIndexSpecificImpl(metadata, dstIndPrefix.data());
            }
            break;
        case UIT_WORD_HOST:
            Impl.Reset(new TImplInvRaw(metadata, dstIndPrefix));
            break;
        default:
            ythrow yexception() << "Bad IndexType: " << static_cast<int>(metadata.IndexType);
    }
}

TQEFlexIndex4Indexing::TQEFlexIndex4Indexing(const TQEFlexIndexMetaData& metadata,
                                             TString dstIndPrefix,
                                             size_t /*maxMemUsageInMb*/,
                                             bool callInit)
    : Finished(false)
{
    if (callInit)
        Init(metadata, dstIndPrefix);
}

TQEFlexIndex4Indexing::~TQEFlexIndex4Indexing()
{
    Finish();
}

void TQEFlexIndex4Indexing::AddGenData(const char* data, size_t size)
{
    Impl->AddGenData(data, size);
}

void TQEFlexIndex4Indexing::NextKeyRaw(const char* rawKey, const char* data, size_t size)
{
    Impl->NextKeyRaw(rawKey, data, size);
}

void TQEFlexIndex4Indexing::NextKeyRaw(const char* rawKey, const TBuffer& data)
{
    NextKeyRaw(rawKey, data.Data(), data.Size());
}

void TQEFlexIndex4Indexing::NextKey(const TString& queryUTF8, const char* data, size_t size)
{
    NextKeyRaw(KeyConv.ConvertUTF8(queryUTF8.data(), queryUTF8.size()), data, size);
}

void TQEFlexIndex4Indexing::NextKey(const TString& queryUTF8, const TBuffer& data)
{
    NextKey(queryUTF8, data.Data(), data.Size());
}

void TQEFlexIndex4Indexing::Add(ui32 docId, const char* data, size_t size)
{
    Impl->Add(docId, data, size);
}

void TQEFlexIndex4Indexing::Add(ui32 docId, const TBuffer& data)
{
    Add(docId, data.Data(), data.Size());
}

void TQEFlexIndex4Indexing::Finish()
{
    if (Finished)
        return;
    Impl.Destroy();
    Finished = true;
}

TString TQEFlexIndex4Indexing::FinishAndGetGenData()
{
    return Impl->FinalizeAndGetGenData();
}

TQEFlexIndex4Read::TQEFlexIndex4Read() {}
TQEFlexIndex4Read::~TQEFlexIndex4Read() {}

void TQEFlexIndex4Read::Close()
{
    Impl.Destroy();
}

const TQEFlexIndexMetaData& TQEFlexIndex4Read::GetMetadata() const
{
    if (!Impl)
        ythrow yexception() << "Indexuser not initialized";
    return *(Impl->Metadata);
}

TArrayRef<const char> GetGenData(const TMappedFile& indexDat, const TQEFlexIndexMetaData* metadata)
{
    return TArrayRef<const char>(((const char*)indexDat.getData()) + sizeof(TQEFlexIndexMetaData),
                       metadata->GenDataSize*4);
}

TArrayRef<const char> TQEFlexIndex4Read::GetGenData() const
{
    return ::GetGenData(Impl->IndexDat, Impl->Metadata);
}

void TQEFlexIndex4Read::SetSkipper(TAutoPtr<IQEFlexIndexSkipper> skipper)
{
    Impl->Skipper = skipper;
}

class TQEFlexIndexIterHits;

class TQEFlexIndex4SearchHits : public TImpl4Search
{
    friend class TQEFlexIndexIterHits;

    TQEFlexIndexEntryCodec EntryCodec;
public:
    TQEFlexIndex4SearchHits(const char *indexPrefix)
        : TImpl4Search(indexPrefix)
        , EntryCodec(*Metadata)
    {}

    IQEFlexIndexIterImpl* CreateIterImpl(TQEFlexIndexIter& iter, const char* rawKey) const override;
};


class TQEFlexIndexIterHits : public IQEFlexIndexIterImpl
{
    const TQEFlexIndexEntryCodec& EntryCodec;

    TPosIterator<>                DocIter;

    const char* CurData = nullptr;
    size_t      CurSize = ui64(-1);
public:
    TQEFlexIndexIterHits(const TQEFlexIndex4SearchHits& index,
                         TQEFlexIndexIter& parent,
                         const char* rawKey)
        : IQEFlexIndexIterImpl(parent)
        , EntryCodec(index.EntryCodec)
    {
        if (DocIter.Init(index.IndexDoc, rawKey, RH_DEFAULT)) {
            CurData = (const char*)(index.IndexDat.getData(EntryCodec.DecodeQ(DocIter.Current()).Offset*4));
            DocIter.Next();
            Y_ASSERT(DocIter.Valid());
            CurSize = EntryCodec.DecodeQ(DocIter.Current()).Size*4;
            Parent.Reset(CurData, CurSize);
        }
    }

    void ReadNext() override
    {
        TQEFlexIndexIterHits::Next();
    }
    void Next() override
    {
        DocIter.Next();
        if (DocIter.Valid()) {
            CurData += CurSize;
            TQEFlexIndexDEntry de = EntryCodec.DecodeD(DocIter.Current());
            CurElId = de.ElId;
            CurSize = de.Size*4;
            Parent.Reset(CurData, CurSize);
        }
    }

    bool Valid() const override
    {
        return DocIter.Valid();
    }

    TArrayRef<const char> Data() const override
    {
        Y_ASSERT(CurData);  // CurData MUST be initialized somewhere
        return TArrayRef<const char>(CurData, CurSize);
    }
};


IQEFlexIndexIterImpl* TQEFlexIndex4SearchHits::CreateIterImpl(TQEFlexIndexIter& iter, const char* rawKey) const {
    THolder<TQEFlexIndexIterHits> iterImpl(new TQEFlexIndexIterHits(*this, iter, rawKey));
    if (iterImpl->Valid())
        return iterImpl.Release();
    return nullptr;
}

class TQEFlexIndex4SearchInvRaw : public TQEFlexIndex4SearchInvRawBase
{
public:
    TQEFlexIndex4SearchInvRaw(const char* indexPrefix)
        : TQEFlexIndex4SearchInvRawBase(indexPrefix)
    {}
    TQEFlexIndex4SearchInvRaw(const TMemoryMap& keyMapping, const TMemoryMap& invMapping, const char* indexPrefix)
        : TQEFlexIndex4SearchInvRawBase(keyMapping, invMapping, indexPrefix)
    {}
    IQEFlexIndexIterImpl* CreateIterImpl(TQEFlexIndexIter& iter, const char* rawKey) const override;
};

class TQEFlexIndexIterInvRaw : public TQEFlexIndexIterInvRawBase
{
public:
    TQEFlexIndexIterInvRaw(TQEFlexIndexIter& parent,
                           const TBlob& dataHolder,
                           const IQEFlexIndexSkipper& skipper)
        : TQEFlexIndexIterInvRawBase(parent, skipper)
    {
        DataHolder = dataHolder;
        parent.Reset(DataHolder.Data(), DataHolder.Size());
    }

};

IQEFlexIndexIterImpl* TQEFlexIndex4SearchInvRaw::CreateIterImpl(TQEFlexIndexIter& iter, const char* rawKey) const
{
    TBlob blob;
    bool success = FindBlob(rawKey, blob);
    return success ? new TQEFlexIndexIterInvRaw(iter, blob, *Skipper) : nullptr;
}

bool TQEFlexIndex4Search::Init(const char *indexPrefix, TSetSkipperFunc setSkipperFunc) {
    try {
        TQEFlexIndexMetaData metadata;
        LoadMetadata4Search(indexPrefix, metadata);

        switch (metadata.IndexType) {
            case UIT_WEB:
                if (metadata.IndexVersion >= 9)
                    CreateIndexSpecificImpl(metadata, indexPrefix);
                if (metadata.IndexVersion >= 4 && metadata.IndexVersion < 9) {
                    InitImpl<TQEFlexIndex4SearchInvRaw>(Impl, indexPrefix);
                }
                if (metadata.IndexVersion < 4) {
                    Impl.Reset(new TQEFlexIndex4SearchHits(indexPrefix));
                }
                break;
            case UIT_IMG:
                if (metadata.IndexVersion <= 5) {
                    Impl.Reset(new TQEFlexIndex4SearchHits(indexPrefix));
                }
                else {
                    CreateIndexSpecificImpl(metadata, indexPrefix);
                }
                break;
            case UIT_VIDEO:
                if (metadata.IndexVersion <= 5) {
                    Impl.Reset(new TQEFlexIndex4SearchHits(indexPrefix));
                } else {
                    CreateIndexSpecificImpl(metadata, indexPrefix);
                }
                break;
            case UIT_WORD_HOST:
                Impl.Reset(new TQEFlexIndex4SearchInvRaw(indexPrefix));
                break;
            default:
                ythrow yexception() << "Bad IndexType: " << metadata.IndexType;
        }

        if (setSkipperFunc)
            setSkipperFunc(*this);
    } catch (const yexception& e) {
        SEARCH_ERROR << "An exception occurred during " << indexPrefix << " QEFlexIndex initialization: " << e.what();
        return false;
    }

    return true;
}

bool LoadMetadata4Search(const char* indexPrefix, TQEFlexIndexMetaData& metadata) {
    const TString path(indexPrefix + TString("dat"));
    if (NFs::Exists(path)) {
        TIFStream in(path);
        LoadPodType(&in, metadata);
        return true;
    }
    return false;
}

IQEFlexIndexIterImpl* TQEFlexIndex4Search::CreateIterImpl(TQEFlexIndexIter& iter, const char* rawKey) const
{
    return ((TImpl4Search*) Impl.Get())->CreateIterImpl(iter, rawKey);
}

class TQEFlexIndex4SeqReadInvRaw : public TImpl4SeqRead
{
public:
    TQEFlexIndex4SeqReadInvRaw(const char* indexPrefix)
        : TImpl4SeqRead(indexPrefix)
    {}

    IQEFlexIndexIterImpl* CreateIterImpl(TQEFlexIndexIter& iter) const override;
};

IQEFlexIndexIterImpl* TQEFlexIndex4SeqReadInvRaw::CreateIterImpl(TQEFlexIndexIter& iter) const
{
    TBlob blob;
    const YxKey& key = IndexDoc.CurKey();
    IndexDoc.GetYndex().GetBlob(blob, key.Offset, key.Length, RH_DEFAULT);
    return new TQEFlexIndexIterInvRaw(iter, blob, *Skipper);
}


bool TQEFlexIndex4SeqRead::Init(const char *indexPrefix, TSetSkipperFunc setSkipperFunc)
{
    try {
        TQEFlexIndexMetaData metadata;
        {
            TIFStream in((TString(indexPrefix) + "dat").data());
            LoadPodType(&in, metadata);
        }

        switch (metadata.IndexType) {
            case UIT_WEB:
                if (metadata.IndexVersion >= 9)
                    CreateIndexSpecificImpl(metadata, indexPrefix);
                if (metadata.IndexVersion >= 4 && metadata.IndexVersion < 9) {
                    Impl.Reset(new TQEFlexIndex4SeqReadInvRaw(indexPrefix));
                }
                if (metadata.IndexVersion < 4) {
                    Impl.Reset(new TQEFlexIndex4SeqReadInvRaw(indexPrefix));
                }
                break;
            case UIT_WORD_HOST:
                Impl.Reset(new TQEFlexIndex4SeqReadInvRaw(indexPrefix));
                break;
            case UIT_VIDEO:
                if (metadata.IndexVersion >= 6) {
                    CreateIndexSpecificImpl(metadata, indexPrefix);
                } else {
                    ythrow yexception() << "Unimplemented for version: " << metadata.IndexVersion;
                }
                break;
            case UIT_IMG:
            default:
                ythrow yexception() << "Bad IndexType: " << metadata.IndexType;
        }
        if (setSkipperFunc)
            setSkipperFunc(*this);
    } catch (yexception& /*e*/) {
        return false;
    }
    return true;
}

bool TQEFlexIndex4SeqRead::Valid() const
{
    return ((TImpl4SeqRead*) Impl.Get())->Valid();
}

void TQEFlexIndex4SeqRead::Next()
{
    ((TImpl4SeqRead*) Impl.Get())->Next();
}

const YxKey& TQEFlexIndex4SeqRead::CurKey()
{
    return ((TImpl4SeqRead*) Impl.Get())->CurKey();
}

IQEFlexIndexIterImpl* TQEFlexIndex4SeqRead::CreateIterImpl(TQEFlexIndexIter& iter) const
{
    return ((TImpl4SeqRead*) Impl.Get())->CreateIterImpl(iter);
}

TQEFlexIndexIter::TQEFlexIndexIter()
    : TZCMemoryInput(nullptr,0)
{}

TQEFlexIndexIter::~TQEFlexIndexIter() {}

bool TQEFlexIndexIter::InitRaw(const TQEFlexIndex4Search& index, const char* rawKey)
{
    Impl.Reset(((TImpl4Search*) index.Impl.Get())->CreateIterImpl(*this, rawKey));
    return Impl.Get() != nullptr;
}

bool TQEFlexIndexIter::Init(const TQEFlexIndex4SeqRead& index)
{
    Impl.Reset(((TImpl4SeqRead*) index.Impl.Get())->CreateIterImpl(*this));
    return Impl.Get() != nullptr;
}

bool TQEFlexIndexIter::Init(const TQEFlexIndex4Search& index, const TStringBuf& keyUTF8) {
    TFormToKeyConvertor keyConv;
    const char* rawKey;
    try {
        rawKey = keyConv.ConvertUTF8(keyUTF8.data(), keyUTF8.size());
    } catch (TKeyConvertException& /*e*/) {
        return false;
    }
    return InitRaw(index, rawKey);
}


void TQEFlexIndexIter::Next()
{
    return Impl->Next();
}

void TQEFlexIndexIter::ReadNext()
{
    return Impl->ReadNext();
}

bool TQEFlexIndexIter::Valid() const
{
    return Impl.Get() && Impl->Valid();
}

ui32 TQEFlexIndexIter::ElId() const
{
    return ui32(Impl->CurElId); // no check for speed
}

TArrayRef<const char> TQEFlexIndexIter::Data() const
{
    return Impl->Data();
}

