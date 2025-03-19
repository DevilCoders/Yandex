#pragma once

/*
 */
#include <util/system/defaults.h>
#include <util/generic/string.h>
#include <kernel/search_types/search_types.h>
#include <util/generic/ptr.h>

#include <kernel/keyinv/hitlist/invsearch.h>
#include <kernel/keyinv/hitlist/positerator.h>

#include "searchfile.h"
#include <util/generic/noncopyable.h>

class TKeysPool;

struct YxKey {
    const char *Text;
    ui32 Length;
    i64 Offset;
    i64 Counter;
};

#define HITS_MAP_SIZE (1 << 18)

class TSequentKeyPosReader : TNonCopyable
{
private:
    const IKeysAndPositions *Yndex;
    THolder<TKeysPool> KPool;
    i32 LowNumKey;
    i32 HighNumKey;
    i32 CurNumKey;
    i32 YBlock;
    TString KeyLow;
    TString KeyHigh;
    TRequestContext RC;
    TBlob SuperHits;
private:
    void NextPortion();
public:
    TSequentKeyPosReader(const IKeysAndPositions* y = nullptr, const char *keyLow = "", const char *keyHigh = "",
            ui32 mapSize = HITS_MAP_SIZE, size_t segsize = MAXKEY_BUF*512);

    ~TSequentKeyPosReader();

    void Init(const IKeysAndPositions *y, const char *keyPreffix,
            ui32 mapSize = HITS_MAP_SIZE, size_t segsize = MAXKEY_BUF*512);
    void Init(const IKeysAndPositions *y, const char *keyLow, const char *keyHigh,
            ui32 mapSize = HITS_MAP_SIZE, size_t segsize = MAXKEY_BUF*512);
    void Restart();

    bool Valid() const;
    const YxKey &CurKey() const;
    void InitIterator(TPosIterator<>& iter);

    void Next();

    const TSubIndexInfo& GetSubIndexInfo() const {
        return Yndex->GetSubIndexInfo();
    }
    const IKeysAndPositions& GetYndex() const {
        return *Yndex;
    }
};

class TSequentPosIterator : public TPosIterator<>
{
public:
    TSequentPosIterator(TSequentKeyPosReader& reader) {
        reader.InitIterator(*this);
    }
};

class TYndex4Searching;

class TSequentYandReader : public TSequentKeyPosReader
{
private:
    THolder<TYndex4Searching> Yndex;
public:
    void Init(const char* file, const char *keyPreffix,
            ui32 mapSize = HITS_MAP_SIZE, size_t segsize = MAXKEY_BUF*512,
            READ_HITS_TYPE defaultReadHitsType = RH_DEFAULT);
    void Init(const char* file, const char *keyLow, const char *keyHigh,
            ui32 mapSize = HITS_MAP_SIZE, size_t segsize = MAXKEY_BUF*512,
            READ_HITS_TYPE defaultReadHitsType = RH_DEFAULT);
    i64 GetInvLength() const {
        return Yndex->GetInvLength();
    }
};
