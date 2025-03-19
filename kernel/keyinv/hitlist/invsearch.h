#pragma once

#include <util/system/defaults.h>
#include <util/generic/yexception.h>
#include <util/generic/vector.h>
#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/memory/blob.h>

#include <kernel/keyinv/invkeypos/keychars.h>
#include <library/cpp/yappy/yappy.h>

#include "record.h"
#include "subindex.h"
#include "hitformat.h"

// unknown block number for the first time call to EntryByNumber etc..
#define UNKNOWN_BLOCK -1

struct TIndexInfo {
    TSubIndexInfo SubIndexInfo;
    ui32 Version;
    bool HasUtfKeys;
    bool StrictHitsOrder;
    TIndexInfo()
        : Version(YNDEX_VERSION_CURRENT)
        , HasUtfKeys(true)
        , StrictHitsOrder(true)
    {}
};

struct TRecordInterval // [FirstNumber, LastNumber)
{
    TRecordInterval()
    : FirstBlock(UNKNOWN_BLOCK)
    , FirstNumber(-1)
    , LastNumber(-1)
    , StartOffset(0)
    , TotalLength(0)
    , TotalCounter(0)
    {}

    i32 FirstBlock;
    i32 FirstNumber;
    i32 LastNumber;
    i64 StartOffset;
    i64 TotalLength;
    i64 TotalCounter;
};

class TRequestContext : TNonCopyable
{
public:
    TRequestContext()
        : Data(new char[REQ_CTX_BUF_SIZE + 32])
        , DataPtr(nullptr)
        , Block(UNKNOWN_BLOCK)
        , KeyCount(0)
        , FirstKey(0)
        , CurrentKey(0)
        , FirstOffset(0)
        , Offset(0)
        // for cache
        , PrevKeyPrefix(0)
        , PrevMaxInterval(0)
        // for unwind
        , UnwindMode(false)
        , CanUnwind(false)
        , FIDataPtr(nullptr)
        , FICurrentKey(0)
        , FIOffset(0)
    {
        Data[0] = 0;
    }
    int GetKeyNumber() const {
        return CurrentKey;
    }
    int GetBlockNumber() const {
        return Block;
    }
    int GetFirstKeyNumber() const {
        return FirstKey;
    }
    YxRecord& GetRecord() {
        return Rec;
    }
    const YxRecord& GetRecord(size_t ii) {
        return RecInterval[ii];
    }
    void ClearInterval() {
        RecInterval.clear();
    }
    void ClearUnwindState() {
        CanUnwind = false;
    }
    size_t IntervalSize() const {
        return RecInterval.size();
    }
    void AddRecordToInterval(const YxRecord& rec) {
        RecInterval.push_back(rec);
    }
    // interval caching mechanism
    bool GetCachedInterval(ui64 keyPrefix, const TString& lemma, ui32 maxInterval, TRecordInterval& res) const {
        if (PrevKeyPrefix != keyPrefix)
            return false;
        if (PrevLemma != lemma)
            return false;
        if (PrevMaxInterval != maxInterval)
            return false;
        res = PrevInterval;
        return true;
    }
    void SetCachedInterval(ui64 keyPrefix, const TString& lemma, ui32 maxInterval, const TRecordInterval& interval) {
        PrevKeyPrefix = keyPrefix;
        PrevLemma = lemma;
        PrevMaxInterval = maxInterval;
        PrevInterval = interval;
    }

    // unwind mechanism
    void SetUnwindMode(bool value) {
        UnwindMode = value;
    }

    void SetBlock(const char* data, int dataSize, int block, int firstKey, int keyCount, ui64 firstOffset, bool yapped) {
        if (yapped) {
            const ui8 *ptr = (const ui8 *)data;
            size_t compSize = ((ui32)ptr[0]) + ((ui32 )ptr[1]) * 256;
            size_t realSize = YappyUnCompress(data + 2, compSize, Data.Get());
            Y_UNUSED(realSize);
            assert(realSize <= REQ_CTX_BUF_SIZE);

        } else {
            dataSize = REQ_CTX_BUF_SIZE;
            assert(dataSize <= REQ_CTX_BUF_SIZE);
            memcpy(Data.Get(), data, dataSize);
        }

        Block = block;
        KeyCount = keyCount;
        FirstKey = firstKey;
        FirstOffset = firstOffset;
        Reset();
    }
    template<class TKeyUnpacker>
    i32 MoveToKey(const char* key, const TIndexInfo& indexInfo) {
        int cmpRes = CompareKeyWithLemma(Rec.TextPointer, key);
        if (cmpRes == 0)
            return CurrentKey;
        if (cmpRes > 0) {
            // промахнулись, попробуем поковыряться в кэше начала предыдущего
            bool unwind = Unwind();
            if (unwind) {
                cmpRes = CompareKeyWithLemma(Rec.TextPointer, key);
                if (cmpRes == 0)
                    return CurrentKey;
                if (cmpRes > 0)
                    Reset();
            } else
                Reset();
        }

        TKeyUnpacker unpacker(Rec.TextPointer);
        int number = FirstKey + KeyCount - 1;
        while (CurrentKey < number) {
            Next(unpacker, indexInfo.SubIndexInfo);
            if (unpacker.Cmp(key) >= 0) {
                // make Rec be valid
                unpacker.AddTail();
                return CurrentKey;
            }
        }
        unpacker.AddTail();
        return FirstKey + KeyCount;
    }
    template<class TKeyUnpacker>
    const YxRecord* MoveToNumber(i32 number, const TIndexInfo& indexInfo) {
        Y_ASSERT((number >= FirstKey && number < FirstKey + KeyCount));
        if (CurrentKey == number) {
            if (!CanUnwind) {
                // TODO: remove code duplication below
                FICurrentKey = CurrentKey;
                FIDataPtr = DataPtr;
                FIRec = Rec;
                FIOffset = Offset;
                CanUnwind = true;
            }
            return &Rec;
        }
        if (CurrentKey > number)
            Reset();

        TKeyUnpacker unpacker(Rec.TextPointer);
        while (CurrentKey < number) {
            Next(unpacker, indexInfo.SubIndexInfo);
        }
        // make Rec be valid
        unpacker.AddTail();
        if (!CanUnwind) {
            FICurrentKey = CurrentKey;
            FIDataPtr = DataPtr;
            FIRec = Rec;
            FIOffset = Offset;
            CanUnwind = true;
        }
        return &Rec;
    }
    // @param lemma      a searched lemma, it must not contain characters less or equal 0x20 except the last character that can be equal to 0x0F
    static int CompareKeyWithLemma(const char* key, const char* lemma) {
        const bool q = (*key == LEMMA_LANG_PREFIX);

        // @todo if (Y_LIKELY(!q))
        if (!q)
            return strcmp(key, lemma);

        // for backward compatibility
        // variants of TextPointer with old keys (see also notes in library/invkeypos/keycode.cpp):
        // lemma x00
        // lemma x01 forms x00
        // lemma x02 lang x00 -> ? lang lemma x00
        // lemma x02 lang x01 forms x00 -> ? lang lemma x01 forms x00
        const char* const p = key + 2;
        const int res = strcmp(p, lemma);
        if (res == 0)
            return (q ? 1 : 0);

        return res;
    }
    static int CompareKeyWithLemma(const char* start, size_t startLen, const char* tail, const char* lemma) {
        if (!startLen)
            return CompareKeyWithLemma(tail, lemma);

        Y_ASSERT(startLen > 1);
        const char* s = start;
        size_t len = startLen;
        const bool q = (*s == LEMMA_LANG_PREFIX);
        if (q) {
            Y_ASSERT(len > 1);
            s += 2;
            len -= 2;
        }
        int n = strncmp(s, lemma, len); // here len can be equal to 0
        if (n != 0)
            return n;
        n = strcmp(tail, lemma + len);
        if (n == 0)
            return (q ? 1 : 0);
        return n;
    }
private:
    void Reset() {
        DataPtr = Data.Get();
        Offset = FirstOffset;
        Rec.TextPointer[0] = 0;
        CurrentKey = FirstKey - 1;
        CanUnwind = false;
    }
    bool Unwind() {
        if (!UnwindMode)
            return false;
        if (!CanUnwind)
            return false;
        DataPtr = FIDataPtr;
        Rec = FIRec;
        CurrentKey = FICurrentKey;
        Offset = FIOffset;
        CanUnwind = false;
        return true;
    }
    template<class TKeyUnpacker>
    void Next(TKeyUnpacker& unpacker, const TSubIndexInfo& si) {
        DataPtr = unpacker.UnpackKey(Rec, DataPtr);
        Rec.Offset = Offset;
        Offset += hitsSize(Offset, Rec.Length, Rec.Counter, si);
        ++CurrentKey;
    }
private:
    enum {
        REQ_CTX_BUF_SIZE = 4096, // BLOCK_SIZE
    };
    TArrayHolder<char> Data;
    const char* DataPtr;
    YxRecord Rec;
    int Block;
    int KeyCount;
    int FirstKey;
    int CurrentKey;
    ui64 FirstOffset;
    ui64 Offset;

    // Records interval for further MultRead-ing
    TVector<YxRecord> RecInterval;

    // interval cache
    ui64 PrevKeyPrefix;
    TString PrevLemma;
    ui32 PrevMaxInterval;
    TRecordInterval PrevInterval;

    // unwind cache
    bool UnwindMode; // unwind enabled or not
    bool CanUnwind; // current unwind state
    const char* FIDataPtr;
    YxRecord FIRec;
    int FICurrentKey;
    ui64 FIOffset;
};

enum READ_HITS_TYPE {
    RH_DEFAULT,
    RH_FORCE_MAP,
    RH_FORCE_ALLOC
};

class IKeyPosScanner;

class IKeysAndPositions {
public:
    virtual void GetBlob(TBlob& data, i64 offset, ui32 length, READ_HITS_TYPE type) const = 0;
    virtual const YxRecord* EntryByNumber(TRequestContext &rc, i32 , i32 &) const = 0; // for sequential access (avoid LowerBound())
    virtual i32 LowerBound(const char *word, TRequestContext &rc) const = 0; // for group searches [...)
    virtual const TIndexInfo& GetIndexInfo() const = 0;

    virtual ui32 KeyCount() const {
        Y_FAIL("Not implemented");
    }

    virtual ~IKeysAndPositions() {}

    ui32 GetVersion() const {
        return GetIndexInfo().Version;
    }

    const TSubIndexInfo& GetSubIndexInfo() const {
        return GetIndexInfo().SubIndexInfo;
    }

    void Scan(IKeyPosScanner& scanner) const;
};

inline const YxRecord* EntryByNumber(const IKeysAndPositions* y, TRequestContext &rc, i32 number)
{
    i32 block = UNKNOWN_BLOCK;
    return y->EntryByNumber(rc, number, block);
}

inline const YxRecord* ExactSearch(const IKeysAndPositions* y, TRequestContext &rc, const char *what)
{
    i32 num = y->LowerBound(what, rc);
    const YxRecord* record = EntryByNumber(y, rc, num);
    if (record && strcmp(record->TextPointer, what) == 0)
        return record;
    return nullptr;
}

struct TLowerBoundCache {
    TString LastCmp;
    i32 Num;
    i32 Block;
    TLowerBoundCache()
        : Num(-1)
        , Block(-1)
    {
    }
};

class TUnorderedKeysException : public yexception {
};

void GetLemmaInterval(const IKeysAndPositions* y, TRequestContext &rc, ui64 prefix, const TString& lemma, ui32 maxInterval, TRecordInterval* intervalToGet, TLowerBoundCache *lowerBoundCache = nullptr);
