#include "hitbuffermanager.h"

THitsBufferManager::~THitsBufferManager() {
    for (TList<TPageInfo>::iterator i = Cache.begin(), e = Cache.end(); i != e; ++i) {
        for (TList<THitsBufferCaller*>::iterator io = i->Owners.begin(), eo = i->Owners.end(); io != eo; ++io) {
            (*io)->BufferManagerCookie = nullptr;
        }
        delete[] i->Data;
    }
    Cache.clear();
}

void THitsBufferManager::ReleasePage(THitsBufferCaller* caller) {
    if (caller->BufferManagerCookie != nullptr) {
        TPageInfo* pageInfo = (TPageInfo*)(caller->BufferManagerCookie);
        pageInfo->Owners.remove(caller);
    }
    caller->BufferManagerCookie = nullptr;
}

size_t THitsBufferManager::GetPage(THitsBufferCaller* caller, NIndexerCore::TInputFile* file, ui32 page, size_t pageSize, char*& pageData) {
    ReleasePage(caller);
    TPageInfo* pageInfo = FindPage(file, page, pageSize);
    if (!pageInfo) {
        pageInfo = GetFreePage(pageSize);
        FetchPage(pageInfo, file, page);
    }
    pageData = pageInfo->Data;
    return RegisterPage(caller, pageInfo);
}

