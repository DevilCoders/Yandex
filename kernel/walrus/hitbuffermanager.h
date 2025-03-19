#pragma once

#include <util/generic/list.h>
#include <util/generic/hash_set.h>

#include <kernel/keyinv/indexfile/filestream.h>
#include <kernel/keyinv/indexfile/rdkeyit.h>

class THitsBufferManager : public IHitsBufferManager {

    struct TPageInfo {
        char*                               Data;           // page data in memory
        NIndexerCore::TInputFile*           File;           // file from which data was read
        ui32                                BytesRead;      // number of bytes that were read
        ui32                                Page;           // page number from the start of file
        size_t                              PageSize;       // page size
        TList<THitsBufferCaller*>           Owners;         // hit iterators that own this page

        TPageInfo(ui32 pageSize)
            : Data(new char[pageSize])
            , PageSize(pageSize)
        {}

        TPageInfo(NIndexerCore::TInputFile* file, ui32 page, size_t pageSize)
            : File(file)
            , Page(page)
            , PageSize(pageSize)
        {}
    };

    struct TPageInfoPtrHash {
        size_t operator()(const TPageInfo* page) const {
            return (page->File->GetHash() << 12) + (size_t)page->Page + size_t(page->PageSize);
        }
    };

    struct TPageInfoPtrEq {
        bool operator()(const TPageInfo* a, const TPageInfo *b) const {
            return a->File == b->File && a->Page == b->Page && a->PageSize == b->PageSize;
        }
    };

    typedef THashSet<TPageInfo*, TPageInfoPtrHash, TPageInfoPtrEq> TReverseCacheLookup;

private:
    TList<TPageInfo> Cache;
    TReverseCacheLookup ReverseCacheLookup;

    static const size_t CACHE_SIZE = 50;

private:
    TPageInfo* FindPage(NIndexerCore::TInputFile* file, ui32 page, size_t pageSize) const;
    TPageInfo* GetFreePage(size_t pageSize);
    size_t RegisterPage(THitsBufferCaller* caller, TPageInfo* pageInfo);
    void FetchPage(TPageInfo* pageInfo, NIndexerCore::TInputFile* file, ui32 page);

public:

    size_t GetPage(THitsBufferCaller* caller, NIndexerCore::TInputFile* file, ui32 page, size_t pageSize, char*& pageData) override;
    void ReleasePage(THitsBufferCaller* caller) override;

    ~THitsBufferManager() override;
};

inline THitsBufferManager::TPageInfo* THitsBufferManager::FindPage(NIndexerCore::TInputFile* file, ui32 page, size_t pageSize) const {
    TPageInfo tmp(file, page, pageSize);
    TReverseCacheLookup::const_iterator it = ReverseCacheLookup.find(&tmp);
    if (it != ReverseCacheLookup.end()) {
        return *it;
    }
    return nullptr;
}

inline THitsBufferManager::TPageInfo* THitsBufferManager::GetFreePage(size_t pageSize) {
    // erase oldest released page
    if (Cache.size() > CACHE_SIZE) {
        for (TList<TPageInfo>::iterator it = Cache.begin(), end = Cache.end(); it != end; ++it) {
            if ((*it).Owners.empty() && (*it).PageSize == pageSize) {
                TPageInfo* pageInfo = &(*it);
                ReverseCacheLookup.erase(pageInfo);
                delete[] pageInfo->Data;
                Cache.erase(it);
                break;
            }
        }
    }
    Cache.push_back(TPageInfo(pageSize));
    return &Cache.back();
}

inline void THitsBufferManager::FetchPage(TPageInfo* pageInfo, NIndexerCore::TInputFile* file, ui32 page) {
    file->Seek(page * (SUPERLONG)pageInfo->PageSize, SEEK_SET);
    pageInfo->BytesRead = file->Read(pageInfo->Data, pageInfo->PageSize);
    pageInfo->File = file;
    pageInfo->Page = page;
    ReverseCacheLookup.insert(TReverseCacheLookup::value_type(pageInfo));
}

inline size_t THitsBufferManager::RegisterPage(THitsBufferCaller* caller, TPageInfo* pageInfo) {
    pageInfo->Owners.push_back(caller);
    caller->BufferManagerCookie = (void*)pageInfo;
    return pageInfo->BytesRead;
}

