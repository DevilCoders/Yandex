#pragma once

#include "multipart_storage.h"

#include <kernel/multipart_archive/config/config.h>

#include <util/generic/ptr.h>
#include <util/system/condvar.h>
#include <util/system/event.h>

namespace NRTYArchive {

    class TStorage {
    public:

        using TPtr = TAtomicSharedPtr<TStorage>;

        class TDocument : public TAtomicRefCount<TDocument> {
        public:
            using TPtr = TIntrusivePtr<TDocument>;
            inline TDocument(TStorage& storage, const TMultipartStorage::TDocInfo& docInfo, TBlob blob)
                : Storage(storage)
                , DocInfo(docInfo)
                , Blob(blob.DeepCopy())
                , Confirmed(false)
                , Version(storage.Version)
            {}

            inline ~TDocument() {
                Storage.FinishDoc(DocInfo, Confirmed, Version);
            }

            inline const TBlob& GetBlob() const {
                return Blob;
            }

            inline void Ack() {
                Confirmed = true;
            }

        private:
            TStorage& Storage;
            const TMultipartStorage::TDocInfo DocInfo;
            TBlob Blob;
            bool Confirmed;
            ui64 Version;
        };

        struct TLimits {
            TLimits(ui32 docs = Max<ui32>(), ui64 bytes = Max<ui64>(), ui32 docsInWork = Max<ui32>())
                : Docs(docs)
                , Bytes(bytes)
                , DocsInWork(docsInWork)
            {}

            ui32 Docs;
            ui64 Bytes;
            ui32 DocsInWork;
        };

        TStorage(const TFsPath& path, const TLimits& limits,  const TMultipartConfig& config);
        ~TStorage();
        TDocument::TPtr Get(bool block);
        void Put(const TBlob& document);
        void Stop();
        void WaitStopped();
        ui32 GetDocsCount() const;
        ui64 GetSizeInBytes() const;
        void CollectGarbage(const TDuration& interDocWait);
        void Clear();
        void Flush() {
            Archive->Flush();
        }


        template <class TActor>
        void ScanNotSafe(TActor& actor) {
            auto iterator = Archive->CreateCommonIterator(new TRawIteratorFactory());
            while (iterator->IsValid()) {
                ui32 docid = iterator->GetDocid();
                TBlob doc = iterator->GetDocument();
                if (docid != TMultipartStorage::DOC_REMOVED && doc.Size() != 0) {
                    actor(doc);
                }
                iterator->Next();
            }
        }

    private:
        ui32 GetAccessedDocsUnsafe() const;
        void UpdateCounters();
        void FinishDoc(const TMultipartStorage::TDocInfo& docInfo, bool ack, ui64 version);
    private:
        THolder<TMultipartStorage> Archive;
        TMultipartStorage::TIteratorPtr Iterator;
        TLimits Limits;
        volatile bool Stopped;
        mutable TAtomic CachedDocsCount;
        mutable TAtomic CachedSizeInBytes;
        mutable TAtomic AccessedDocsCount;
        ui64 Version;

        TMutex Mutex;
        TRWMutex ClearLock;
        TManualEvent StopEvent;
        TSet<TMultipartStorage::TDocInfo> DocIdsInWork;
    };

}
