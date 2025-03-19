#pragma once
#include <kernel/search_types/search_types.h>
#include "qeflexindex_1.h"

class TQEFlexIndex4Indexing::IImpl
{
protected:
    TString DstIndPrefix;

    struct TDocEntry
    {
        ui32        Id;
        TArrayRef<const char> Data;

        TDocEntry(ui32 id = 0, TArrayRef<const char> data = TArrayRef<const char>())
          : Id(id)
          , Data(data)
        {}

        bool operator<(const TDocEntry& rhs) const
        {
            return Id < rhs.Id;
        }
    };

    TString                  CurKeyRaw;
    segmented_pool<char>    CurData;
    TArrayRef<const char>             CurKeyData;
    TVector<TDocEntry>      CurDocsData;

    TFixedBufferFileOutput     IndexDatS;
    ui64                    CurDatOffset; // in 4-byte blocks

    TQEFlexIndexMetaData    Metadata;

    //
    static const char Dummy[3];// = {'\0'};   // padding

public:
    IImpl(const TQEFlexIndexMetaData& metadata, TString dstIndPrefix)
        : DstIndPrefix(dstIndPrefix)
        , CurKeyRaw("")
        , CurData(10*1024)
        , IndexDatS(dstIndPrefix + "dat")
        , CurDatOffset(0)
        , Metadata(metadata)
    {}

    void Finalize() {
        if (CurDatOffset == 0) { // header hasn't been saved yet
            AddGenData(nullptr, 0);
        }
        if (!CurKeyRaw.empty()) {
            SaveCurKey();
        }
        IndexDatS.Finish();
    }

    virtual void FinalizeImpl() = 0;

    virtual ~IImpl()
    {
    }

    void SaveCurKey()
    {
        std::sort(CurDocsData.begin(), CurDocsData.end());

        SaveCurKeyImpl();

        CurDocsData.clear();
        CurData.clear();
        CurKeyRaw.clear();
    }
    virtual void AddGenData(const char* data, size_t size)
    {
        if (CurDatOffset != 0) {
            ythrow yexception() << "TQEFlexIndex4Indexing::AddGenData: Some data already saved";
        }
        ui32 sizeIn4 = (size / 4) + (size % 4 ? 1 : 0);
        if (sizeIn4 > Max<ui16>()) {
            ythrow yexception() << "TQEFlexIndex4Indexing::AddGenData: data size " <<  (ui64)size << " is too big";
        }
        Metadata.GenDataSize = (ui16)sizeIn4;
        SaveDataPadded(TArrayRef<const char>(reinterpret_cast<const char*>(&Metadata), sizeof(TQEFlexIndexMetaData)));
        if (sizeIn4) {
            SaveDataPadded(TArrayRef<const char>(data, size));
        }
    }

    void NextKeyRaw(const char* rawKey, const char* data, size_t size)
    {
        Y_ASSERT(*rawKey && strlen(rawKey) < MAXKEY_BUF);
        if (!CurKeyRaw.empty()) {
            Y_ASSERT(TStringBuf(rawKey) > CurKeyRaw);
            SaveCurKey();
        }
        CurKeyRaw = rawKey;
        CurKeyData = TArrayRef<const char>(size ? CurData.append(data, size) : nullptr, size);
    }

    void Add(ui32 docId, const char* data, size_t size)
    {
        CurDocsData.push_back(TDocEntry(docId, TArrayRef<const char>(size ? CurData.append(data, size) : nullptr, size)));
    }

    // returns sizeIn4
    ui32 SaveDataPadded(TArrayRef<const char> data)
    {
        ui32 sizeIn4 = (data.size() / 4) + (data.size() % 4 ? 1 : 0);
        ui32 remSize = sizeIn4*4 - data.size();
        IndexDatS.Write(data.data(), data.size());
        if (remSize) {
            IndexDatS.Write(Dummy, remSize);
        }
        CurDatOffset += sizeIn4;
        return sizeIn4;
    }

    virtual TString FinalizeAndGetGenData() {
        FinalizeImpl();
        TMappedFile indexDat(DstIndPrefix);
        return TString(((const char*)indexDat.getData()) + sizeof(TQEFlexIndexMetaData),
                           CurDatOffset * 4);
    }

    virtual void SaveCurKeyImpl() = 0;
};

class TImplInvRawBase : public TQEFlexIndex4Indexing::IImpl
{
protected:
    NIndexerCore::TOutputIndexFileImpl<TFile> IndexFile;

    void WriteInv(TArrayRef<const char> data, i64& len)
    {
        IndexFile.WriteInvData(data.data(), data.size());
        len += i64(data.size());
    }

public:
    TImplInvRawBase(const TQEFlexIndexMetaData& metadata,
                TString dstIndPrefix)
      : IImpl(metadata, dstIndPrefix)
      , IndexFile(IYndexStorage::FINAL_FORMAT,
                  YNDEX_VERSION_RAW64_HITS | YNDEX_VERSION_FLAG_KEY_ADDITIONALY_COMPRESSED_YAPPY)
    {
        IndexFile.Open(dstIndPrefix.data());
    }

    void FinalizeImpl() override {
        Finalize();
        NIndexerCore::WriteFastAccessTable(IndexFile, false);
    }

};

class TQEFlexIndex4Read::IImpl
{
    friend class TQEFlexIndex4Read;
    friend class IQEFlexIndexIterImpl;
protected:
    TMappedFile                     IndexDat;
    const TQEFlexIndexMetaData*     Metadata;
    TAutoPtr<IQEFlexIndexSkipper>   Skipper;
public:
    IImpl(const char *indexPrefix)
        : IndexDat((TString(indexPrefix) + "dat").data())
    {
        Metadata = (const TQEFlexIndexMetaData*)(IndexDat.getData());
    }

    virtual ~IImpl()
    {
    }
};

class TImpl4Search : public TQEFlexIndex4Read::IImpl
{
    friend class TQEFlexIndex4Search;
protected:
    TYndex4Searching                IndexDoc;
public:
    TImpl4Search(const char* indexPrefix)
        : TQEFlexIndex4Read::IImpl(indexPrefix)
    {
        IndexDoc.InitSearch(indexPrefix);
    }
    TImpl4Search(const TMemoryMap& keyMapping, const TMemoryMap& invMapping, const char* indexPrefix)
        : TQEFlexIndex4Read::IImpl(indexPrefix)
    {
        IndexDoc.InitSearch(keyMapping, invMapping);
    }

    ~TImpl4Search() override
    {
        IndexDoc.CloseSearch();
    }

    virtual IQEFlexIndexIterImpl* CreateIterImpl(TQEFlexIndexIter& iter, const char* rawKey) const = 0;
};

class TImpl4SeqRead : public TQEFlexIndex4Read::IImpl
{
    friend class TQEFlexIndex4SeqRead;
protected:
    TSequentYandReader              IndexDoc;
public:
    TImpl4SeqRead(const char *indexPrefix)
        : TQEFlexIndex4Read::IImpl(indexPrefix)
    {
        char highKey[MAXKEY_BUF];
        memset(highKey, '\xff', MAXKEY_BUF-1);
        highKey[MAXKEY_BUF-1] = '\0';
        IndexDoc.Init(indexPrefix, "", highKey);
    }

    bool Valid() {
        return IndexDoc.Valid();
    }

    void Next() {
        IndexDoc.Next();
    }

    const YxKey& CurKey() {
        return IndexDoc.CurKey();
    }
    virtual IQEFlexIndexIterImpl* CreateIterImpl(TQEFlexIndexIter& iter) const = 0;
};


class TQEFlexIndex4SearchInvRawBase : public TImpl4Search
{
protected:
    bool FindBlob(const char* rawKey, TBlob& blob, i32* elNum = nullptr) const;
public:
    TQEFlexIndex4SearchInvRawBase(const char* indexPrefix)
        : TImpl4Search(indexPrefix)
    {}
    TQEFlexIndex4SearchInvRawBase(const TMemoryMap& keyMapping, const TMemoryMap& invMapping, const char* indexPrefix)
        : TImpl4Search(keyMapping, invMapping, indexPrefix)
    {}
};

class IQEFlexIndexIterImpl
{
    friend class TQEFlexIndexIter;
protected:
    TQEFlexIndexIter&                 Parent;

    i32                               CurElId; // could be -1 initially
public:
    IQEFlexIndexIterImpl(TQEFlexIndexIter& parent)
        : Parent(parent)
        , CurElId(-1)
    {}

    virtual ~IQEFlexIndexIterImpl() {}

    virtual void ReadNext() = 0;  // we've read all data for current, move to next element
    virtual void Next() = 0; // move to next element (if no reading functions were called, only Data())
    virtual bool Valid() const = 0;
    virtual TArrayRef<const char> Data() const = 0;
};


class TQEFlexIndexIterInvRawBase : public IQEFlexIndexIterImpl
{
protected:
    const IQEFlexIndexSkipper& Skipper;
    TBlob DataHolder;
public:
    TQEFlexIndexIterInvRawBase(TQEFlexIndexIter& parent, const IQEFlexIndexSkipper& skipper)
        : IQEFlexIndexIterImpl(parent)
        , Skipper(skipper)
    {}
    void ReadNext() override
    {
        if (Valid()) {
            ui32 elIdDiff;
            UnPackUI32(Parent, elIdDiff);
            if (CurElId == -1) {
                CurElId = (i32)elIdDiff;
            } else {
                CurElId += (i32)elIdDiff;
            }
        }
    }

    void Next() override
    {
        if (CurElId == -1) {
            Skipper.SkipQData(Parent);
        } else {
            Skipper.SkipQEData(Parent);
        }
        TQEFlexIndexIterInvRawBase::ReadNext();
    }

    bool Valid() const override
    {
        return Parent.Avail();
    }

    TArrayRef<const char> Data() const override
    {
        TZCMemoryInput mi(Parent.Buf(), Parent.Avail());
        if (CurElId == -1) {
            Skipper.SkipQData(mi);
        } else {
            Skipper.SkipQEData(mi);
        }
        return TArrayRef<const char>(Parent.Buf(), Parent.Avail() - mi.Avail());
    }
};


template <class TIndex>
void InitImpl(THolder<TQEFlexIndex4Read::IImpl>& impl, const char* indexPrefix) {
    const TMemoryMap* keyMapping = GetMappedIndexFile(TString::Join(indexPrefix, "key"));
    const TMemoryMap* invMapping = GetMappedIndexFile(TString::Join(indexPrefix, "inv"));
    if (keyMapping && invMapping)
        impl.Reset(new TIndex(*keyMapping, *invMapping, indexPrefix));
    else
        impl.Reset(new TIndex(indexPrefix));
}

