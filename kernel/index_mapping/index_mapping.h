#pragma once

#include <library/cpp/streams/special/throttled.h>

#include <util/system/filemap.h>
#include <util/datetime/base.h>

/// Obtains mapping (or NULL if global index mapping disabled or mapping failed)
const TMemoryMap* GetMappedIndexFile(const TString& fileName);

/// Removes @c fileName from global mapping
void ReleaseMappedIndexFile(const TString& fileName);
void UnlockMappedIndexFile(const TString& fileName);

/// Removes all mapped files under @c dir from global mapping
void ReleaseMappedIndexFilesInDir(const TString& dir);
void UnlockMappedIndexFilesInDir(const TString& dir);

/// Enables entire global mapping engine
void EnableGlobalIndexFileMapping();

// Enable ThrottledLockMemory which limits reading
void EnableThrottledReading(TThrottle::TOptions readOptions);

/// Create mappings with read-write policy
void EnableWritableMapping();

/// Create mappings with prefetched memory
void EnablePrechargedMapping();

//Set default options in TMappedFiles
void ResetMappedFiles();

bool IsIndexFileLocked(const TString& fileName);

struct TIndexPrefetchOptions {
    TIndexPrefetchOptions()
        : Lower(0.0f)
        , Upper(1.0f)
        , PrefetchLimit(size_t(-1))
        , TryLock(false)
        , TryRead(true)
        , Deadline(TInstant::Max())
    {
    }

    float Lower; // lower prefetch border, from 0.0 to 1.0
    float Upper; // upper prefetch border, from 0.0 to 1.0
    size_t PrefetchLimit; // bytes
    bool TryLock; // call LockMemory
    bool TryRead; // walk through memory, can be stopped by deadline
    TInstant Deadline;
};

struct TIndexPrefetchResult {
    void* Addr = nullptr; // for unlocking
    size_t Prefetched = 0; // prefetched size
    bool TimedOut = false;
    bool Locked = false;
};

TIndexPrefetchResult PrefetchMappedIndex(const TString& fileName, const TIndexPrefetchOptions& opts);
namespace NMappedFiles {
    ui32 GetMappedFilesCount();
    ui32 GetLockedFilesCount();
    bool HasMappedIndexFile(const TString& fileName);
    bool HasLockedIndexFile(const TString& fileName);
}
