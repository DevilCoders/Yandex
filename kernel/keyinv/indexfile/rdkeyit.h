#pragma once

#include <kernel/search_types/search_types.h>
#include <cerrno>
#include <cstdint>

#include <util/generic/algorithm.h>

#include <util/system/filemap.h>
#include <util/generic/buffer.h>

#include <kernel/keyinv/hitlist/hits_coders.h>

#include "fat.h"
#include "indexfile.h"
#include "indexreader.h"
#include <util/generic/noncopyable.h>

class THitsBufferCaller;

class IHitsBufferManager {
public:

    virtual size_t GetPage(THitsBufferCaller* caller, NIndexerCore::TInputFile* file, ui32 page, size_t pageSize, char*& pageData) = 0;
    virtual void ReleasePage(THitsBufferCaller* caller) = 0;

    virtual ~IHitsBufferManager() = default;
};

class THitsBufferCaller {
protected:
    void* BufferManagerCookie = nullptr;

    THitsBufferCaller() = default;

    friend class THitsBufferManager;
};

// fields needed for heap usages of TBufferedHitIterator
struct THitsHeapFields {
    uintptr_t       ArrIndex;
    int             FormsMap[N_MAX_FORMS_PER_KISHKA];
};

/**
 * It's essential that this constant is the SAME
 * for the pair TUnsortedKeyInvWriter -- T(Unsorted)BufferedHitIterator
 * Seems it's better not to change it at all.
 **/
static const size_t UNSORTED_HIT_BUFFER_SIZE = 0x2000;

class TBufferedHitIterator : private THitsBufferCaller, public THitsHeapFields, TNonCopyable {
public:
    static const size_t DEF_PAGE_SIZE_BITS  = 22;
    static const size_t PHIT_MAX            = 128; // maximum size of packed hit

    typedef SUPERLONG value_type;


    typedef DecoderFallBack<CHitDecoder, false, HIT_FMT_BLK8> THitDecoder;

protected:
    NIndexerCore::TInputFile* InvFile;

    const size_t        PageSize;
    const size_t        PageSizeBits;

    IHitsBufferManager* BufferManager;
    char*               PageData;                   // pointer to a page loaded into memory

    ui32                CurrentPage;                // current page index
    char*               PageEnd;                    // end of PageData
    char*               RealPageEnd;                // end of bytes read into PageData

    char                EdgeBuffer[PHIT_MAX * 2];   // buffer used for temporarily storing hits that cross page boundaries
    ui32                EdgeBufferPrevPage;         // index of the previous page that edge buffer bridges

    const char*         Cur;                        // next readable hit
    const char*         Upper;                      // up to where we can read hits without crossing page boundaries
    const char*         End;                        // end of kishka

    bool                IsLength64;
    ui64                End64;

    bool                IsOver;

    THitDecoder         HitDecoder;

    // memorized positions for Rewind()
    SUPERLONG       LastStart;
    ui32            LastLength;
    ui32            LastCount;

private:
    void InitBuffers(IHitsBufferManager* bufferManager);

    void GetPageReal(ui32 page);
    void CheckPageReal(ui32 page);
    void FillEdgeBuffer(ui32 page);
    void CheckPage();

public:
    TBufferedHitIterator(size_t pageSizeBits = DEF_PAGE_SIZE_BITS);
    ~TBufferedHitIterator();

    void Init(NIndexerCore::TInputFile& invFile, EHitFormat hitFormat = HIT_FMT_BLK8, IHitsBufferManager* bufferManager = nullptr);

    inline void Restart(SUPERLONG start, ui32 length, ui32 count, bool memorize = false);
    void Restart64(SUPERLONG start, SUPERLONG lenth, ui32 count);

    void Rewind() {
        Restart(LastStart, LastLength, LastCount);
    }

    void Drain() {
        Upper = End = Cur;
        IsLength64 = false;
        IsOver = true;
        LastLength = 0;
    }


    inline bool ReadPackedI64(i64 &value) {
        if (Y_UNLIKELY(Cur >= Upper)) {
            if (Cur >= End) {
                IsOver = false;
                return false;
            }
            CheckPage();
        }
#if (defined(_unix_) && defined(_i386_)) && !defined(_cygwin__) && !defined(_darwin_)
        ASMi386_UNPACK_64(Cur, value);
#elif (defined(_unix_) && defined(__x86_64__)) && !defined(_cygwin__) && !defined(_darwin_)
        ASMi64_UNPACK_64(Cur, value);
#else
        int ret; // not used actually
        Y_UNUSED(ret);
        UNPACK_64(value, Cur, mem_traits, ret);
#endif
        return true;
    }

    inline bool NextI64() {
        if (Y_UNLIKELY(Cur >= Upper)) {
            if (Cur >= End) {
                IsOver = true;
                return false;
            }
            CheckPage();
        }
#if (defined(_unix_) && defined(__x86_64__)) && !defined(_cygwin__) && !defined(_darwin_)
        SUPERLONG newCurrent = HitDecoder.GetCurrent();
        ASMi64_UNPACK_ADD_64(Cur, newCurrent);
        HitDecoder.SetCurrent(newCurrent);
        //SUPERLONG diff;
        //ASMi64_UNPACK_64(Cur, diff);
        //hitDecoder.currentExternal += diff;
#else
        SUPERLONG diff;
        int ret; // not used actually
        Y_UNUSED(ret);
        UNPACK_64(diff, Cur, mem_traits, ret);
        HitDecoder.SetCurrent(HitDecoder.GetCurrent() + diff);
#endif
        return true;
    }

    ui8* NextSaved();
    inline bool Next();

    void operator++() {
        Next();
    }

    bool operator <(const TBufferedHitIterator& other) const {
        if (IsOver)
            return false;
        else if (other.IsOver)
            return true;
        else
            return HitDecoder.GetCurrent() < other.HitDecoder.GetCurrent();
    }

    // For compatibility
    bool Valid() const {
        return !IsOver;
    }

    inline SUPERLONG Current() const {
        return HitDecoder.GetCurrent();
    }

    inline SUPERLONG operator *() {
        return HitDecoder.GetCurrent();
    }

    void Restart() {}

    SUPERLONG GetFullOffset() const {
        return CurrentPage != (ui32)-1 ? CurrentPage * (SUPERLONG)PageSize + (Cur - PageData) : -1;
    }

    void SetCurrentHit(i64 value) {
        HitDecoder.SetCurrent(value);
    }

    void FillHeapData(const TBufferedHitIterator& other) {
        ArrIndex = other.ArrIndex;
    }

    size_t GetPageSizeBits() const {
        return PageSizeBits;
    }

    // some strange friends
    friend int collect_idx_stat(int argc, char *argv[]);
    friend int test_skipto(int argc, char *argv[]);

private:
    void Y_FORCE_INLINE DoNext() {
        HitDecoder.Next(Cur, End);
    }
};

// made inline to hint the compiler
inline void TBufferedHitIterator::Restart(SUPERLONG start, ui32 length, ui32 count, bool memorize) {
    if (memorize) {
        LastStart = start;
        LastLength = length;
        LastCount = count;
    }
    if (!length) {
        IsOver = true;
        return;
    }

    ui32 page = (ui32)(start >> PageSizeBits); //64: this may be a problem in a far future
    const char* current = PageData + (start & (PageSize - 1));
    if (Y_UNLIKELY(CurrentPage != page || length >= PageSize || current + length > PageEnd)) {
        Restart64(start, length, count);
        return;
    }
    // most usual case: we are entirely within current page
    Cur = current;
    Upper = End = current + length;
    IsOver = ((uintptr_t) Cur >= ((uintptr_t) Cur) + length);

    IsLength64 = false;
    HitDecoder.Reset();
    HitDecoder.SetSize(count);
    HitDecoder.ReadHeader(Cur);
    if (count < 8)
        HitDecoder.FallBackDecode(Cur, count);
}

inline bool TBufferedHitIterator::Next() {

    if (HitDecoder.Fetch()){
        return true;
    } else if (Y_UNLIKELY(Cur >= Upper)) {
        if (Cur >= End) {
            IsOver = true;
            return false;
        }
        CheckPage();
    }
    DoNext();
    return true;
}

// fields needed for heap iterator
struct TKeysHeapFields {
    size_t HiTitersCount;
    TKeysHeapFields()
        : HiTitersCount(0)
    {
    }
};

template<typename HitIteratorType = TBufferedHitIterator>
class TReadKeysIteratorImpl : public TKeysHeapFields, TNonCopyable {
    NIndexerCore::TInputIndexFile IndexFile;
    THolder<NIndexerCore::TInvKeyReader> IndexReader;

protected:
    bool RepositionAtNumber(const TFastAccessTable& fat, i32 number);

public:
    typedef HitIteratorType THitIterator;
    THitIterator Hits;

    void SetHitIteratorIndex(size_t i) {
        Hits.ArrIndex = i;
    }

public:
    //! @note if format is equal to PORTION_FORMAT version is equal to YNDEX_VERSION_CURRENT
    //!       if format is equal to FINAL_FORMAT version and hasSubIndex are read from inv-file
    TReadKeysIteratorImpl(IYndexStorage::FORMAT format = IYndexStorage::PORTION_FORMAT)
        : IndexFile(format)
    {
    }

    TReadKeysIteratorImpl(IYndexStorage::FORMAT format, size_t iterPageSizeBits)
      : IndexFile(format)
      , Hits(iterPageSizeBits)
    {
    }

    //! @note in case of PORTION_FORMAT version and hasSubIndex must be specified explicitly
    //!       for FINAL_FORMAT they can have any values because they will be read from inv-file
    TReadKeysIteratorImpl(IYndexStorage::FORMAT format, ui32 version, bool /*hasSubIndex*/,
        size_t iterPageSizeBits = THitIterator::DEF_PAGE_SIZE_BITS)
        : IndexFile(format, version)
        , Hits(iterPageSizeBits)
    {
    }

    void Init(const char* keyName, const char* invName, IHitsBufferManager* bufferManager = nullptr, bool directIO = false) {
        const size_t bufSize = Max<size_t>(1024 * 1024, 1 << (Hits.GetPageSizeBits() - 2));
        IndexFile.Open(keyName, invName, bufSize, 0, directIO);
        IndexReader.Reset(new NIndexerCore::TInvKeyReader(IndexFile));
        Hits.Init(IndexFile.GetInvFile(), IndexReader->GetHitFormat(), bufferManager);
    }

    void Init(const char* keyName, const char* invName, size_t keyBufSize, size_t invBufSize, IHitsBufferManager* bufferManager = nullptr, bool directIO = false) {
        IndexFile.Open(keyName, invName, keyBufSize, invBufSize, directIO);
        IndexReader.Reset(new NIndexerCore::TInvKeyReader(IndexFile));
        Hits.Init(IndexFile.GetInvFile(), IndexReader->GetHitFormat(), bufferManager);
    }

    void Init(const char* prefix, IHitsBufferManager* bufferManager = nullptr, bool directIO = false) {
        TString keyName(TString::Join(prefix, NIndexerCore::KEY_SUFFIX));
        TString invName(TString::Join(prefix, NIndexerCore::INV_SUFFIX));
        Init(keyName.data(), invName.data(), bufferManager, directIO);
    }

    ui32 GetYndexVersion() const  {
        return IndexFile.GetVersion();
    }

    void Close() {
        IndexReader.Reset(nullptr);
        IndexFile.Close();
    }

    int LowerBound(const char* prefixKey);

    bool Next() {
        if (!NextKey())
            return false;
        NextHit();
        return true;
    }

    bool NextKey() {
        return IndexReader->ReadNext();
    }

    void NextHit(bool memorize = false) {
        NextHit(Hits, memorize);
    }

    void NextHit(THitIterator& hitsIterator, bool memorize = false) {
        hitsIterator.Restart(IndexReader->GetOffset(), GetHitsLength<HIT_FMT_BLK8>(
            IndexReader->GetLength(), IndexReader->GetCount(), IndexReader->GetHitFormat()),
            IndexReader->GetCount(), memorize);
    }

    bool operator>(const TReadKeysIteratorImpl& other) const {
        return strcmp(GetKeyText(), other.GetKeyText()) > 0;
    }

    const char* Str() const {
        return IndexReader->GetKeyText();
    }

    const char* GetKeyText() const {
        return Str();
    }

    i64 GetCount() const {
        return IndexReader->GetCount();
    }

    ui32 GetLength() const {
        return IndexReader->GetLength();
    }

    i64 GetOffset() const {
        return IndexReader->GetOffset();
    }

    ui32 GetSizeOfHits() const {
        return IndexReader->GetSizeOfHits();
    }

    EHitFormat GetHitFormat() const {
        return IndexReader->GetHitFormat();
    }

    NIndexerCore::TInputFile& GetInvFile() {
        return IndexFile.GetInvFile();
    }
};

template<typename THitIterator>
bool TReadKeysIteratorImpl<THitIterator>::RepositionAtNumber(const TFastAccessTable& fat, i32 number) {
    i32 block = UNKNOWN_BLOCK;
    fat.GetBlock(number, block);

    const int firstKeyInBlock = fat.FirstKeyInBlock(block);
    number -= firstKeyInBlock;

    IndexReader->SkipTo(fat.GetKeyOffset(block), fat.GetOffset(block));

    for (int i = 0; i < number; ++i)
        if (!IndexReader->ReadNext())
            return false;

    return true;
}


template<typename THitIterator>
int TReadKeysIteratorImpl<THitIterator>::LowerBound(const char* prefixKey)
{
    assert(IndexFile.GetFormat() == IYndexStorage::FINAL_FORMAT);

    TIndexInfo indexInfo;
    TFastAccessTable fat;
    try {
        indexInfo = fat.Open(IndexFile);
    } catch (...) {
        int err = errno;
        warnx("FastAccess");
        return err;
    }

    TRequestContext rc;
    TFileMap keyFileMap(IndexFile.CreateKeyFileMap());
    i32 startNumKey = ::FatLowerBound(fat, indexInfo, keyFileMap, prefixKey, rc);
    if (startNumKey < 0) {
        warnx("StartNumKey < 0 (for '%s')", prefixKey);
        if (fat.KeyCount() == 0) //possible bad verification but will work on empty index
            return 0;
        else
            return 1;
    }

    if (!RepositionAtNumber(fat, startNumKey)) {
        warnx("RepositionAtNumber < 0");
        return 1;
    }
    return 0;
}

// Usage:
// 1) Call NextKey(), key is in .Str(), hits are in [0.. .Size())
//    NextKey() returns .Size(), so if 0 we have finished
// 2) For hits Heap, call ReadFirstHit() and then construct MultHitHeap<TBufferedHitIterator>(&.[0], .Size())
//    Don't forget to call MultHitHeap::Restart() after that

// required members for template parameters if NextKey() used
//     THitIterator: Next(), ArrIndex (actually it isn't used)
//     TKeyIterator: Hits, operator>(const TKeyIterator&), Str(), NextHit(bool)
template <typename THitIterator = TBufferedHitIterator, typename TKeyIterator = TReadKeysIteratorImpl<THitIterator> >
class THeapOfHitIteratorsImpl : public TVector<THitIterator*> {
protected:
    struct TPtrGreater {
        bool operator() (const TKeyIterator* a, const TKeyIterator* b) const {
            return *a > *b;
        }
    };

    typedef TVector<TKeyIterator*> THeap;
    typedef TVector<THitIterator*> TBase;

    THeap                           TheHeap;
    size_t                          CurrentHeapSize;
    size_t                          CurrentSize;
    char                            KeyText[MAXKEY_BUF];

public:
    THeapOfHitIteratorsImpl(TKeyIterator* iters, size_t count);

    const char* Str() const {
        return KeyText;
    }

    size_t Size() const {
        return CurrentSize;
    }

    size_t NextKey(bool memorize = false) {
        if (!CurrentHeapSize) {
            CurrentSize = 0;
            return 0;
        }

        CurrentSize = 0;
        TKeyIterator* top = TheHeap[0];
        strcpy(KeyText, top->Str());

        do {
            PopHeap(&TheHeap[0], &TheHeap[0] + CurrentHeapSize, TPtrGreater());
            top->NextHit(memorize);
            if (top->NextKey())
                PushHeap(&TheHeap[0], &TheHeap[0] + CurrentHeapSize, TPtrGreater());
            else
                CurrentHeapSize--;
            (*this)[CurrentSize++] = &top->Hits;
        } while (CurrentHeapSize && !strcmp(Str(), (top = TheHeap[0])->Str()));

        return CurrentSize;
    }

    void ReadFirstHit() {
        for (size_t n = 0; n < CurrentSize; n++) {
            (*this)[n]->Next();
        }
    }
};

template <typename THitIterator, typename TKeyIterator>
THeapOfHitIteratorsImpl<THitIterator, TKeyIterator>::THeapOfHitIteratorsImpl(TKeyIterator* iters, size_t count)
    : TVector<THitIterator*>(count)
    , TheHeap(count)
{
    CurrentHeapSize = 0;
    for (size_t n = 0; n < count; n++) {
        if (iters[n].NextKey()) {
            TheHeap[CurrentHeapSize++] = iters + n;
            iters[n].SetHitIteratorIndex(n);
        }
    }
    MakeHeap(TheHeap.begin(), TheHeap.begin() + CurrentHeapSize, TPtrGreater());
    CurrentSize = 0;
    *KeyText = 0;
}

using TReadKeysIterator = TReadKeysIteratorImpl<TBufferedHitIterator>;
using THeapOfHitIterators = THeapOfHitIteratorsImpl<TBufferedHitIterator>;

template <class Task>
void ScanSeqKeys(const TString& index, const char* lowKey, Task &task)
{
    TReadKeysIterator ri(IYndexStorage::FINAL_FORMAT);
    ri.Init(index.data());

    ri.LowerBound(lowKey);
    while (ri.NextKey()) {
        if (task.BeforeKey(ri.Str())) {
            ri.NextHit();
            while (ri.Hits.Next()) {
                SUPERLONG Cur = ri.Hits.Current();
                task.Do(Cur);
            }
            task.AfterKey();
        }
    }
}
