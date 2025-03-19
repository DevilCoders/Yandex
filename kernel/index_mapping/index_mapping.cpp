#include "index_mapping.h"

#include <kernel/searchlog/errorlog.h>

#include <util/string/cast.h>
#include <util/folder/dirut.h>
#include <util/folder/path.h>
#include <util/generic/map.h>
#include <util/generic/singleton.h>
#include <util/generic/maybe.h>
#include <util/system/mutex.h>
#include <util/system/file.h>
#include <util/system/info.h>
#include <util/system/mlock.h>

namespace NGlobalIndexMappingPrivate {
    typedef TAtomicSharedPtr<TMemoryMap> TSharedMapping;
    typedef TMap<TString, TSharedMapping> TMappings;
    typedef TMap<TString, TIndexPrefetchResult> TLockedFiles;

    struct TMappedFiles {
        bool Enabled;
        TMemoryMap::EOpenMode Policy;
        TMappings Mappings;
        TLockedFiles LockedFiles;
        TMutex Mutex;
        TMaybe<TThrottle::TOptions> ThrottleReadOptions;

        void SetDefaultOptions() {
            Enabled = false;
            Policy = TMemoryMap::oRdOnly;
            ThrottleReadOptions.Clear();
        }

        TMappedFiles() {
            SetDefaultOptions();
        }

        ~TMappedFiles() {
            ResetLockedFiles();
            ResetMappings();
        }
        
        void ResetMappings();
        void ResetLockedFiles();
    };

    inline void TMappedFiles::ResetMappings() {
        for (auto& it : Mappings) {
            SEARCH_WARNING << "Mappings is not clear: " << it.first << Endl;
        }
        Mappings.clear();
    }

    void TMappedFiles::ResetLockedFiles() {
        for (auto& it : LockedFiles) {
            SEARCH_WARNING << "LockedFiles is not clear: " << it.first << Endl;
            if (it.second.Locked) {
                try {
                    UnlockMemory(it.second.Addr, it.second.Prefetched);
                } catch (...) {
                    SEARCH_ERROR << "Cannot unlock " << it.first << " " << it.second.Addr << " " << it.second.Prefetched << ": " << CurrentExceptionMessage();
                }
            }
        }
        LockedFiles.clear();
    }

    TMappedFiles& GetMappedIndexFiles() {
        return *Singleton<TMappedFiles>();
    }

    inline TString SanitizePath(const TString& fileName) {
        return TFsPath(fileName).Fix().GetPath();
    }
} // namespace NGlobalIndexMappingPrivate

using namespace NGlobalIndexMappingPrivate;

namespace NMappedFiles {

    ui32 GetMappedFilesCount() {
        const TMappedFiles& mf = GetMappedIndexFiles();
        return mf.Mappings.size();
    }

    ui32 GetLockedFilesCount() {
        const TMappedFiles& mf = GetMappedIndexFiles();
        return mf.LockedFiles.size();
    }

    bool HasMappedIndexFile(const TString& fileName) {
        TMappedFiles& mf = GetMappedIndexFiles();
        TGuard<TMutex> lock(mf.Mutex);

        if (!mf.Enabled) {
            return false;
        }

        const TString& path = SanitizePath(fileName);
        return mf.Mappings.contains(path);
    }

    bool HasLockedIndexFile(const TString& fileName) {
        TMappedFiles& mf = GetMappedIndexFiles();
        TGuard<TMutex> lock(mf.Mutex);

        if (!mf.Enabled) {
            return false;
        }

        const TString& path = SanitizePath(fileName);
        return mf.LockedFiles.contains(path);
    }

}

void ResetMappedFiles() {
    TMappedFiles& mf = GetMappedIndexFiles();
    TGuard<TMutex> lock(mf.Mutex);
    mf.ResetLockedFiles();
    mf.ResetMappings();
    mf.SetDefaultOptions();
}

bool IsIndexFileLocked(const TString& fileName) {
    const TMappedFiles& mf = GetMappedIndexFiles();
    TGuard<TMutex> lock(mf.Mutex);

    if (!mf.Enabled) {
        return false;
    }
    TString path = SanitizePath(fileName);
    auto prefResult = mf.LockedFiles.FindPtr(path);
    return prefResult && prefResult->Locked;
}

const TMemoryMap* GetMappedIndexFile(const TString& fileName) {
    TMappedFiles& mf = GetMappedIndexFiles();
    TGuard<TMutex> lock(mf.Mutex);

    if (!mf.Enabled) {
        return nullptr;
    }

    const TString& path = SanitizePath(fileName);
    TSharedMapping& m = mf.Mappings[path];
    if (!m) {
        try {
            TSharedMapping mapping(new TMemoryMap(path, mf.Policy));
            m = mapping;
        } catch (...) {
            SEARCH_ERROR << "Can't load " << path.Quote() <<
                " to index mapping: " << CurrentExceptionMessage();
            mf.Mappings.erase(path);
            return nullptr;
        }
    }

    return m.Get();
}

void RegisterLockedFile(const TString& fileName, const TIndexPrefetchResult& result) {
    TMappedFiles& mf = GetMappedIndexFiles();
    TGuard<TMutex> lock(mf.Mutex);

    if (!mf.Enabled) {
        return;
    }

    const TString& path = SanitizePath(fileName);
    mf.LockedFiles[path] = result;
}

void ReleaseMappedIndexFileImpl(const TString& fileName) {
    TMappedFiles& mf = GetMappedIndexFiles();
    TGuard<TMutex> lock(mf.Mutex);

    if (!mf.Enabled) {
        return;
    }

    const TString& path = SanitizePath(fileName);
    mf.Mappings.erase(path);
    SEARCH_INFO << "Released cached mapping " << path.Quote();
}

void ReleaseMappedIndexFilesInDirImpl(const TString& dir) {
    TMappedFiles& mf = GetMappedIndexFiles();
    TGuard<TMutex> lock(mf.Mutex);

    if (!mf.Enabled) {
        return;
    }
    TString slashedDir = SanitizePath(dir);
    SlashFolderLocal(slashedDir);

    TMappings::iterator it = mf.Mappings.begin();
    while (it != mf.Mappings.end()) {
        if (it->first.StartsWith(slashedDir)) {
            mf.Mappings.erase(it++);
        } else {
            ++it;
        }
    }
    SEARCH_INFO << "Released cached mappings under " << slashedDir.Quote();
}

void ReleaseMappedIndexFile(const TString& fileName) {
    UnlockMappedIndexFile(fileName);
    ReleaseMappedIndexFileImpl(fileName);
}

void ReleaseMappedIndexFilesInDir(const TString& dir) {
    UnlockMappedIndexFilesInDir(dir);
    ReleaseMappedIndexFilesInDirImpl(dir);
}

void UnlockMappedIndexFile(const TString& fileName) {
    TMappedFiles& mf = GetMappedIndexFiles();
    TGuard<TMutex> lock(mf.Mutex);

    if (!mf.Enabled) {
        return;
    }

    const TString& path = SanitizePath(fileName);
    auto i = mf.LockedFiles.find(path);
    if (i == mf.LockedFiles.end()) {
        return;
    }

    const TIndexPrefetchResult info = i->second;
    if (!info.Locked) {
        Y_VERIFY(!mf.LockedFiles.contains(path));
        return;
    }

    try {
        UnlockMemory(info.Addr, info.Prefetched);
        SEARCH_INFO << "Unlocked " << path.Quote();
    } catch (...) {
        SEARCH_ERROR << "Cannot unlock " << path.Quote() << " " << info.Addr << " " << info.Prefetched << ": " << CurrentExceptionMessage();
    }
    mf.LockedFiles.erase(path);
}

void UnlockMappedIndexFilesInDir(const TString& dir) {
    TMappedFiles& mf = GetMappedIndexFiles();
    TGuard<TMutex> lock(mf.Mutex);

    if (!mf.Enabled) {
        return;
    }

    TString slashedDir = SanitizePath(dir);
    SlashFolderLocal(slashedDir);

    TVector<TString> deleteList;
    for (auto&& i : mf.LockedFiles) {
        if (i.first.StartsWith(slashedDir))
            deleteList.push_back(i.first);
    }

    for (auto&& file : deleteList) {
        UnlockMappedIndexFile(file);
    }

    SEARCH_INFO << "Unlocked files under " << slashedDir.Quote();
}

void EnableThrottledReading(TThrottle::TOptions readOptions) {
    TMappedFiles& mf = GetMappedIndexFiles();
    TGuard<TMutex> lock(mf.Mutex);
    mf.ThrottleReadOptions = readOptions;
}

void EnableGlobalIndexFileMapping() {
    TMappedFiles& mf = GetMappedIndexFiles();
    TGuard<TMutex> lock(mf.Mutex);
    mf.Enabled = true;
}

void EnableWritableMapping() {
    TMappedFiles& mf = GetMappedIndexFiles();
    TGuard<TMutex> lock(mf.Mutex);
    mf.Policy = TMemoryMap::oRdWr | (mf.Policy & ~TMemoryMap::oAccessMask);
}

void EnablePrechargedMapping() {
    TMappedFiles& mf = GetMappedIndexFiles();
    TGuard<TMutex> lock(mf.Mutex);
    mf.Policy |= TMemoryMap::oPrecharge;
}

TIndexPrefetchResult PrefetchMappedIndex(const TString& fileName, const TIndexPrefetchOptions& opts) {
    TIndexPrefetchResult result;

    THolder<TFileMap> fileHolder;
    if (const TMemoryMap* mapping = GetMappedIndexFile(fileName)) {
        fileHolder.Reset(new TFileMap(*mapping));
        SEARCH_INFO << "PrefetchMappedIndex: entry " << fileName.Quote() << " found, loaded from common index mapping";
    } else {
        fileHolder.Reset(new TFileMap(fileName));
        SEARCH_INFO << "PrefetchMappedIndex: entry " << fileName.Quote() << " not found, loaded from file";
    }
    TFileMap& file = *fileHolder;

    const size_t fileSize = size_t(file.Length());

    file.Map(0, fileSize);
    size_t begOffset = Min(size_t(double(fileSize) * opts.Lower), fileSize);
    size_t endOffset = Min(size_t(double(fileSize) * opts.Upper), fileSize);

    const size_t pageSize = NSystemInfo::GetPageSize();
    begOffset = AlignDown(begOffset, pageSize);
    endOffset = AlignDown(endOffset, pageSize);

    endOffset = Min(endOffset, begOffset + opts.PrefetchLimit);

    char* beg = (char *)file.Ptr() + begOffset;
    char* end = (char *)file.Ptr() + endOffset;
    result.Addr = beg;

    if (opts.TryLock) {
        try {
            TMappedFiles& mf = GetMappedIndexFiles();
            TGuard<TMutex> lock(mf.Mutex);
            if (mf.ThrottleReadOptions) {
                try {
                    ThrottledLockMemory(beg, endOffset - begOffset, *(mf.ThrottleReadOptions.Get()));
                } catch(...) {
                    UnlockMemory(beg, endOffset - begOffset);
                    throw;
                }
            } else {
                LockMemory(beg, endOffset - begOffset);
            }
            SEARCH_INFO << fileName << " locked";
            result.Locked = true;
            result.Prefetched = endOffset - begOffset;
        } catch (...) {
            const TString& error = CurrentExceptionMessage();
            SEARCH_ERROR << "Cannot lock " << fileName.Quote() << ": " << error;
        }

        if (result.Locked) {
            RegisterLockedFile(fileName, result);
        }
    }

    if (!result.Locked && opts.TryRead) {
        size_t loop = 0;
        unsigned int sum = 0;
        while (beg < end) {
            sum = sum * 5 + beg[0];
            beg += pageSize;
            result.Prefetched += pageSize;
            if (0 == (loop++ & 0xFFF) && TInstant::Now() > opts.Deadline) {
                SEARCH_WARNING << fileName << " prefetch timeouted";
                result.TimedOut = true;
                return result;
            }
        }

        SEARCH_INFO << fileName << " crc " << sum;
    }

    return result;
}
