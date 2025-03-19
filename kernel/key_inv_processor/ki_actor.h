#pragma once

#include "memory_portions.h"

#include <kernel/walrus/advmerger.h>

#include <kernel/keyinv/indexfile/memoryportion.h>
#include <kernel/keyinv/indexfile/indexfileimpl.h>
#include <library/cpp/wordpos/wordpos.h>

#include <util/generic/string.h>

class TAdvancedMergeTaskWriter {
private:
    TSimpleSharedPtr<TAdvancedMergeTask> Task;
    TMutex Mutex;

public:
    TAdvancedMergeTaskWriter();
    ~TAdvancedMergeTaskWriter();

    TSimpleSharedPtr<TAdvancedMergeTask> GetTask() {
        return Task;
    }

    void AddInput(const TString& prefix, IYndexStorage::FORMAT format);
    void AddOutput(const TString& prefix);
    void BuildRemapTable(const TVector<ui32>* remapTable);
};

class IKIStorage {
public:
    virtual ~IKIStorage() {}
    virtual bool Next(const char*& key, SUPERLONG*& positions, ui32& posCount) = 0;
};

class TRTYKIActor {
private:
    TString Directory;
    TString Prefix;
    ui32 PortionNum;
    ui32 MaxPortionDocs;
    TVector<ui32> DocIds;
    THolder<TMemoryPortionUsage> MPUAttr;
    THolder<TMemoryPortionUsage> MPULemm;
    TAdvancedMergeTaskWriter& MergeTaskWriter;
    bool Working;
    const bool BuildByKeysFlag = false;

public:
    TRTYKIActor(const TString& prefix, const TString& dir, TAdvancedMergeTaskWriter& mergeTaskWriter, ui32 maxPortionDocs, const bool buildByKeysFlag);
    ~TRTYKIActor() {
        VERIFY_WITH_LOG(!Working, "Incorrect portion destructor usage");
    }

    void Start();
    void Stop();

    void Close();
    void Discard();

    bool IncDoc();
    bool StoreDoc(IKIStorage& storage, ui32 docId);
    bool StorePositions(const char* key, SUPERLONG* positions, ui32 posCount);

private:
    void Flush();
};
