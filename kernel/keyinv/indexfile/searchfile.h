#pragma once

#include <util/system/filemap.h>
#include <kernel/keyinv/hitlist/invsearch.h>
#include "fat.h"
#include <util/generic/noncopyable.h>

class TYndex4Searching: public IKeysAndPositions, TNonCopyable {
public:
    TYndex4Searching();
    ~TYndex4Searching() override;

    ui32 KeyCount() const override {
        return Fat.KeyCount();
    }
    const TIndexInfo& GetIndexInfo() const override {
        return IndexInfo;
    }

    void InitSearch(const TString& indexName, READ_HITS_TYPE defaultReadHitsType=RH_DEFAULT);
    void InitSearch(const TString& keyName, const TString& invName, READ_HITS_TYPE defaultReadHitsType=RH_DEFAULT);
    void InitSearch(const TMemoryMap& keyMapping, const TMemoryMap& invMapping, READ_HITS_TYPE defaultReadHitsType = RH_DEFAULT);
    void CloseSearch();

    const YxRecord* EntryByNumber(TRequestContext &rc, i32 , i32 &) const override;
    i32 LowerBound(const char *word, TRequestContext &rc) const override; // for group searches [...)
    const char* WordByNumber(TRequestContext &rc, i32 number) const;
    void GetBlob(TBlob& data, i64 offset, ui32 length, READ_HITS_TYPE type) const override;
    i64 GetInvLength() const;

protected:
    THolder<TMemoryMap> InvFile;
private:
    THolder<TFileMap> KeyFile;
    TFastAccessTable Fat;
    TIndexInfo IndexInfo;
    TString InvFileName;
    READ_HITS_TYPE DefaultReadHitsType = RH_DEFAULT; //Default hits read type if user not specified type in GetBlob
};
