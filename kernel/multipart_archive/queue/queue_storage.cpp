#include <kernel/multipart_archive/abstract/globals.h>
#include <kernel/multipart_archive/iterators/factory.h>
#include "queue_storage.h"

#include <library/cpp/logger/global/global.h>
#include <util/generic/guid.h>

namespace NRTYArchive {
    bool operator<(const TMultipartStorage::TDocInfo& info1, const TMultipartStorage::TDocInfo& info2) {
        return info1.PartNum < info2.PartNum || (info1.PartNum == info2.PartNum && info1.NumInPart < info2.NumInPart);
    }

    TStorage::TStorage(const TFsPath& path, const TLimits& limits, const TMultipartConfig& config)
        : Limits(limits)
        , Stopped(false)
    {
        TFsPath dir = path.Parent();
        if (!dir.Exists())
            dir.MkDirs();
        Archive = MakeHolder<TMultipartStorage>(path, config.CreateReadContext().SetMapHeader(true));
        ui32 docsCount = Archive->GetDocsCount(false);
        AtomicSet(CachedDocsCount, docsCount);
        AtomicSet(CachedSizeInBytes, Archive->GetSizeInBytes());
        AtomicSet(AccessedDocsCount, docsCount);
        AtomicSet(Version, 0);
    }

    TStorage::~TStorage() {
        Stop();
        WaitStopped();

        Iterator.Drop();

        if (Archive->GetDocsCount(false) == 0) {
            TString path = Archive->GetPath();
            Archive.Destroy();
            TFsPath(path + FAT_EXT).ForceDelete();
        }
    }

    void TStorage::UpdateCounters() {
        AtomicSet(CachedDocsCount, Archive->GetDocsCount(false));
        AtomicSet(CachedSizeInBytes, Archive->GetSizeInBytes());
    }

    ui32 TStorage::GetAccessedDocsUnsafe() const {
        return AtomicGet(AccessedDocsCount);
    }

    TStorage::TDocument::TPtr TStorage::Get(bool block) {
        do {
            THolder<TGuard<TMutex>> blockingGuard;
            THolder<TTryGuard<TMutex>> nonBlockingGuard;
            if (block) {
                blockingGuard = MakeHolder<TGuard<TMutex>>(Mutex);
            } else {
                nonBlockingGuard = MakeHolder<TTryGuard<TMutex>>(Mutex);
                if (!nonBlockingGuard->WasAcquired()) {
                    return nullptr;
                }
            }
            if (Stopped)
                return nullptr;

            if (AtomicGet(AccessedDocsCount) == 0 || DocIdsInWork.size() >= Limits.DocsInWork) {
                if (block)
                    continue;
                else {
                    if (DocIdsInWork.size() >= Limits.DocsInWork)
                        DEBUG_LOG << "DocIdsInWork limits exceeded with " << DocIdsInWork.size() << Endl;
                    return nullptr;
                }
            }

            if (!block && (ui64)AtomicGet(AccessedDocsCount) == DocIdsInWork.size())
                return nullptr;

            if (Iterator) {
                Iterator->Next();
                if (!Iterator->IsValid()) {
                    Iterator.Drop();
                }
            }
            if (!Iterator)
                Iterator = Archive->CreateCommonIterator(new TRawIteratorFactory());
            if (!Iterator->IsValid())
                continue;
            ui32 docid = Iterator->GetDocid();
            auto docInfo = Iterator->GetDocumentInfo();
            if (docid == TMultipartStorage::DOC_REMOVED || !DocIdsInWork.insert(docInfo).second)
                continue;
            TBlob doc = Iterator->GetDocument();
            if (doc.Size())
                return new TDocument(*this, docInfo, doc);
            DocIdsInWork.erase(docInfo);
            Archive->RemoveDocument(docInfo.PartNum, docInfo.NumInPart);
            AtomicDecrement(AccessedDocsCount);
            UpdateCounters();
        } while (true);
    }

    void TStorage::Put(const TBlob& document) {
        TReadGuard rg(ClearLock);

        if (Stopped)
            return;
        if (AtomicGet(CachedDocsCount) >= Limits.Docs)
            ythrow yexception() << Archive->GetPath().GetPath() << ": DocsLimit exceeded";
        if (Archive->GetSizeInBytes() >= Limits.Bytes)
            ythrow yexception() << Archive->GetPath().GetPath() << ": Archive Size Limit exceeded";

        Archive->AppendDocument(document);
        AtomicIncrement(AccessedDocsCount);
        UpdateCounters();
    }

    void TStorage::Stop() {
        Stopped = true;
    }

    void TStorage::WaitStopped() {
        while (true) {
            {
               TGuard<TMutex> g(Mutex);
               if (DocIdsInWork.empty())
                   return;
            }
            Sleep(TDuration::MilliSeconds(50));
        }
    }

    void TStorage::Clear() {
        TWriteGuard clearG(ClearLock);
        TGuard<TMutex> g(Mutex);
        Version++;
        ui32 removedDocsCount = Archive->Clear(0);
        for (ui32 i = 0; i < removedDocsCount; ++i)
            AtomicDecrement(AccessedDocsCount);

        DocIdsInWork.clear();
        UpdateCounters();
        Iterator.Drop();
    }

    ui32 TStorage::GetDocsCount() const {
        return AtomicGet(CachedDocsCount);
    }

    ui64 TStorage::GetSizeInBytes() const {
        return AtomicGet(CachedSizeInBytes);
    }

    void TStorage::FinishDoc(const TMultipartStorage::TDocInfo& docInfo, bool ack, ui64 version) {
        if (ack) {
            AtomicDecrement(AccessedDocsCount);
            Archive->RemoveDocument(docInfo.PartNum, docInfo.NumInPart);
            AtomicSet(CachedDocsCount, Archive->GetDocsCount(false));
            AtomicSet(CachedSizeInBytes, Archive->GetSizeInBytes());
        }
        TGuard<TMutex> g(Mutex);
        VERIFY_WITH_LOG(AtomicGet(Version) != version || DocIdsInWork.erase(docInfo), "Invalid docid");
    }

    void TStorage::CollectGarbage(const TDuration&) {
    }
}
