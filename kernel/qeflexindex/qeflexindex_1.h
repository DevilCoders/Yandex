#pragma once

#include <kernel/geo/reg_data.h>
#include <kernel/index_mapping/index_mapping.h>

#include <library/cpp/deprecated/mapped_file/mapped_file.h>
#include <kernel/keyinv/indexfile/indexwriter.h>
#include <kernel/keyinv/indexfile/searchfile.h>
#include <kernel/keyinv/indexfile/seqreader.h>
#include <kernel/keyinv/invkeypos/keyconv.h>
#include <library/cpp/packedtypes/packed.h>
#include <library/cpp/streams/zc_memory_input/zc_memory_input.h>

#include <util/generic/algorithm.h>
#include <util/generic/ptr.h>
#include <util/generic/array_ref.h>
#include <util/generic/vector.h>
#include <util/generic/ylimits.h>
#include <util/memory/segmented_string_pool.h>
#include <util/stream/file.h>
#include <util/system/defaults.h>

#include <cmath>

enum EQEFlexIndexType
{
    UIT_WEB = 0,
    UIT_IMG = 1,
    UIT_VIDEO = 2,
    UIT_WORD_HOST = 3, // for 'quality/factors/reg_host'
};

#pragma pack(1)
struct Y_PACKED TQEFlexIndexMetaData
// size must be a multiple of 4 cause dat offsets are in 4byte clocks
{
    ui8     IndexVersion;
    ui8     IndexType;      // EQEFlexIndexType
    ui16    GenDataSize;    // in 32 bit chunks
    ui64    NDOwn;

    TQEFlexIndexMetaData(ui8 indexVersion = 0, ui8 indexType = 0, ui16 genDataSize = 0, ui64 nDOwn = 0)
        : IndexVersion(indexVersion)
        , IndexType(indexType)
        , GenDataSize(genDataSize)
        , NDOwn(nDOwn)
    {
    }
};
#pragma pack()

static_assert(sizeof(TQEFlexIndexMetaData) == 12, "expect sizeof(TQEFlexIndexMetaData) == 12");

/* Format of kishka is the following:

0:  TQEFlexIndexQEntry (0,0, offset, size)
1:  TQEFlexIndexQEntry (0,0, offset+1, size) // per query data
2:  TQEFlexIndexDEntry (0,1, elid, size)
...
N:  TQEFlexIndexDEntry (0,1, elid, size)
*/

struct TQEFlexIndexQEntry
{
    ui64 Offset;
    ui32 Size;
};

inline IOutputStream& operator<<(IOutputStream& out, const TQEFlexIndexQEntry& e)
{
    out << e.Offset << '\t' << e.Size << Endl;
    return out;
}

struct TQEFlexIndexDEntry
{
    ui32 ElId;
    ui32 Size;
};

inline IOutputStream& operator<<(IOutputStream& out, const TQEFlexIndexDEntry& e)
{
    out << e.ElId << '\t' << e.Size << Endl;
    return out;
}


/* packed format
    [63]                       SignBit:1;
    [62]                       NonDefBit:1; (1 - for DEntry)
    [(SizeFieldWidth+4)..61]   Offset(for QEntry,in ui32 chunks)/ElId(for DEntry):(58-SizeFieldWidth)
    [4..(SizeFieldWidth+3)]    Size(in ui32 chunks):SizeFieldWidth
    [0..3]                     Dummy:4;
    */

class TQEFlexIndexEntryCodec
{
    static const ui64 NONDEF_BIT  = 0x4000000000000000LL;
    static const ui64 SIZE_SHIFT   = 4;

    ui64 NonSizeMask;
    ui64 NonSizeShift;
    ui64 SizeMask;

public:
    TQEFlexIndexEntryCodec(const TQEFlexIndexMetaData& metadata);

    SUPERLONG Code(const TQEFlexIndexQEntry& qe) const
    {
        Y_ASSERT((qe.Offset & ~NonSizeMask) == 0);
        Y_ASSERT((qe.Size & ~SizeMask) == 0);
        return (qe.Offset << NonSizeShift) | (ui64(qe.Size) << SIZE_SHIFT);
    }

    SUPERLONG Code(const TQEFlexIndexDEntry& de) const
    {
        Y_ASSERT((de.ElId & ~NonSizeMask) == 0);
        Y_ASSERT((de.Size & ~SizeMask) == 0);
        return NONDEF_BIT | (ui64(de.ElId) << NonSizeShift) | (ui64(de.Size) << SIZE_SHIFT);
    }

    TQEFlexIndexQEntry DecodeQ(SUPERLONG data) const
    {
        TQEFlexIndexQEntry qe;
        qe.Offset = (data >> NonSizeShift) & NonSizeMask;
        qe.Size = ui32((data >> SIZE_SHIFT) & SizeMask);
        return qe;
    }

    TQEFlexIndexDEntry DecodeD(SUPERLONG data) const
    {
        TQEFlexIndexDEntry de;
        de.ElId = ui32((data >> NonSizeShift) & NonSizeMask);
        de.Size = ui32((data >> SIZE_SHIFT) & SizeMask);
        return de;
    }
};


//////////////////////////////////////////////////////////////////
// Indexing

class TQEFlexIndex4Indexing
{
public:
    class IImpl;

protected:
    THolder<IImpl> Impl;
private:
    TFormToKeyConvertor KeyConv;

    bool Finished; // TODO - it it really needed?
private:
    // This function should be overriden in derived classes if you plan to do some implementation-specific stuff during writing(like packing something for example)
    virtual void CreateIndexSpecificImpl(const TQEFlexIndexMetaData& /*metadata*/, const char* /*indexPrefix*/)
    {
        ythrow yexception() << "CreateIndexSpecificImpl() must be replaced in base class, call of this function indicates a bug somewhere";
    }

public:
    TQEFlexIndex4Indexing(const TQEFlexIndexMetaData& metadata, TString dstIndPrefix,
                        size_t maxMemUsageInMb = 200, bool callInit = true);
    void Init(const TQEFlexIndexMetaData& metadata, TString dstIndPrefix);
    virtual ~TQEFlexIndex4Indexing();

    // general per-index data
    // call before any NextKey and add.
    void AddGenData(const char* data, size_t size);
    TString FinishAndGetGenData();

    void NextKey(const TString& keyUTF8, const char* data, size_t size);
    void NextKey(const TString& keyUTF8, const TBuffer& data);

    void NextKeyRaw(const char* rawKey, const char* data, size_t size);
    void NextKeyRaw(const char* rawKey, const TBuffer& data);

    void Add(ui32 id, const char* data, size_t size);
    void Add(ui32 id, const TBuffer& data);

    void Finish(); // TODO - maybe move it to destructor
};

//////////////////////////////////////////////////////////////////
// Read

// used to skip data, not needed for some index types
struct IQEFlexIndexSkipper
{
    virtual void SkipQData(TZCMemoryInput& data) const = 0;
    virtual void SkipQEData(TZCMemoryInput& data) const = 0;

    virtual ~IQEFlexIndexSkipper() {}
};

// skips whole stream
struct TQEFlexTrivialIndexSkipper : public IQEFlexIndexSkipper
{
    void SkipQData(TZCMemoryInput& data) const override
    {
        data.Skip(data.Avail());
    }

    void SkipQEData(TZCMemoryInput& data) const override
    {
        data.Skip(data.Avail());
    }
};

class TQEFlexIndexIter;
class IQEFlexIndexIterImpl;

class TQEFlexIndex4Read
{
    friend class TQEFlexIndexIter;
    friend class IQEFlexIndexIterImpl;
public:
    class IImpl;

public:
    TQEFlexIndex4Read();
    virtual ~TQEFlexIndex4Read();

    void Close();

    const TQEFlexIndexMetaData& GetMetadata() const;

    void SetSkipper(TAutoPtr<IQEFlexIndexSkipper> skipper);

    TArrayRef<const char> GetGenData() const;
protected:
    friend class IImpl;

    THolder<IImpl> Impl;
protected:
    // This function should be overriden in derived classes if you plan to do some implementation-specific stuff during reading(like unpacking something)
    virtual void CreateIndexSpecificImpl(const TQEFlexIndexMetaData& /*metadata*/, const char* /*indexPrefix*/)
    {
        ythrow yexception() << "CreateIndexSpecificImpl() must be replaced in base class, call of this function indicates a bug somewhere";
    }
};

//////////////////////////////////////////////////////////////////
// Search


class TQEFlexIndex4Search;

typedef void (*TSetSkipperFunc)(TQEFlexIndex4Read& index);


class TQEFlexIndex4Search : public TQEFlexIndex4Read
{
public:
    bool Init(const char *indexPrefix, TSetSkipperFunc setSkipperFunc = nullptr);

    ~TQEFlexIndex4Search() override {}
protected:
    IQEFlexIndexIterImpl* CreateIterImpl(TQEFlexIndexIter& iter, const char* rawKey) const;
};

bool LoadMetadata4Search(const char* indexPrefix, TQEFlexIndexMetaData& metadata);

//////////////////////////////////////////////////////////////////
// Iterating

class TQEFlexIndex4SeqRead : public TQEFlexIndex4Read
{

public:
    bool Init(const char *indexPrefix, TSetSkipperFunc setSkipperFunc = nullptr);

    bool Valid() const;

    void Next();

    const YxKey& CurKey();

    ~TQEFlexIndex4SeqRead() override {};
protected:
    IQEFlexIndexIterImpl* CreateIterImpl(TQEFlexIndexIter& iter) const;
};

/*
  Use as the following:
    if ( it.Init(index,query) ) {
    ...
           read ALL data using TZCMemoryInput functions

           iter.Data call does not count as reading,
               you need to call Skip to move to the next element if you've only called Data()


            // read ALL data using TZCMemoryInput functions
            it.ReadNext(); // move to elements
        // or
            // maybe call it.Data()
            it.Next(); // move to elements

        while (it.Valid()) {
            if (it.ElId() is interesting) {
                    // read ALL data using TZCMemoryInput functions
                    it.ReadNext();
                // or
                    // maybe call it.Data()
                    it.Next();
            } else {
                it.Next();
            }
        }
    }
*/

class TQEFlexIndexIter : public TZCMemoryInput
    // it's public for convinience but you can't Reset or Fill it externally
{
private:
    THolder<IQEFlexIndexIterImpl> Impl;

public:
    TQEFlexIndexIter();
    ~TQEFlexIndexIter() override;

    const IQEFlexIndexIterImpl* GetImpl() const
    {
        return Impl.Get();
    }

    bool Init(const TQEFlexIndex4Search& index, const TStringBuf& keyUTF8);
    bool Init(const TQEFlexIndex4SeqRead& index);
    bool InitRaw(const TQEFlexIndex4Search& index, const char* rawKey);

    void ReadNext(); // we've read all data for current, move to next element
    void Next(); // move to next element (if no reading functions were called, only Data())

    bool Valid() const;

    ui32 ElId() const;

    // it's more effective to read from this as MemoryStream than calling this function
    TArrayRef<const char> Data() const;
};

namespace NUserData {
    class TIndexUserMetaData;
    class TQEFlexIndexGenData;
}

TArrayRef<const char> GetGenData(const TMappedFile& indexDat, const TQEFlexIndexMetaData* metadata);
