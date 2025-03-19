#pragma once

#include "interfaces.h"
#include "archive_manager.h"
#include "part_optimization.h"

#include <kernel/multipart_archive/abstract/archive.h>
#include <kernel/multipart_archive/abstract/part.h>


#include <util/system/mutex.h>
#include <util/generic/vector.h>
#include <util/system/fs.h>

namespace NRTYArchive {

    struct TOffsetAndDocid {
        TOffsetAndDocid() {}

        TOffsetAndDocid(ui64 offset, ui32 docid, ui64 position)
            : Offset(offset)
            , Docid(docid)
            , Position(position) {}

        inline bool operator < (const TOffsetAndDocid& other) const {
            return Offset < other.Offset;
        }

        ui64 Offset = 0;
        ui32 Docid = 0;
        ui64 Position = 0;
    };
    static_assert(sizeof(TOffsetAndDocid) == 24, "");

    class TArchivePartThreadSafe : public TAtomicRefCount<TArchivePartThreadSafe>, public IClosable, public IPartState {
    public:
        typedef TIntrusivePtr<TArchivePartThreadSafe> TPtr;

        class IRepairer {
        public:
            virtual ~IRepairer() {}

            virtual void Flush(const TFsPath&) {}
            virtual void OnIterNext(ui32 docNum, ui64 offset) = 0;
            virtual ui32 GetFullDocsCount() const = 0;
            virtual ui32 GetRepairedDocsCount() const = 0;
            virtual ui32 GetLost() const = 0;
        };

    public:
        TArchivePartThreadSafe(const TFsPath& path, ui32 index, const IArchiveManager* manager, bool writable, IPartCallBack& owner);
        ~TArchivePartThreadSafe();

        // Data operations
        TBlob GetDocument(IArchivePart::TOffset offset) const;

        // returns TPosition::InvalidOffset if the document isn't written
        [[nodiscard]] IArchivePart::TOffset TryPutDocument(const TBlob& document, ui32 docid, ui32* idx = nullptr);

        bool RemoveDocument();
        IArchivePart::IIterator::TPtr CreateSlaveIterator();

        // Header operations
        TArchivePartThreadSafe::TPtr HardLinkOrCopyTo(const TFsPath& path, ui32 index, const IArchiveManager* manager, ui32 newDocsCount, IPartCallBack& owner) const;
        void UpdateHeader(ui64 position, ui32 docid);
        ui32 GetHeaderData(ui64 position) const;

        //
        static bool Repair(const TFsPath& path, ui32 index, IArchivePart::TConstructContext ctx, IRepairer* repairer);
        static bool AllRight(const TFsPath& path, ui32 index);
        static void Remove(const TFsPath& path, ui32 inex);
        static void Rename(const TFsPath& from, const TFsPath& to, ui32 inex);

        void Drop();
        bool IsFull() const;
        ui64 GetSizeInBytes() const override; // IPartState
        ui64 GetPartSizeLimit() const override; // IPartState
        ui64 GetDocsCount(bool withRemoved = false) const override; // IPartState
        virtual void DoClose() override;
        virtual void CloseImpl();
        TFsPath GetPath() const override; // IPartState

        ui32 GetPartNum() const;
        static ui32 ParsePartFromFileName(const TFsPath& file, const TFsPath& archivePrefix);

        bool IsWritable() const {
            return !IsClosed();
        }

    private:
        static void SaveCompressionParams(TPartMetaSaver& ms, const IArchivePart::TConstructContext& ctx);
        IPartCallBack& Owner;
        const IArchiveManager* Manager;
        THolder<IArchivePart> Slave;
        TRWMutex DataMutex;
        TFsPath Path;

        ui32 PartIndex;
        THolder<IPartHeader> PartHeader;
        THolder<IArchivePart::IPolicy> Policy;
        bool Dropped;
        bool IsFullNotified = false;

        TAtomic DocsCount = 0;
        TAtomic RemovedDocsCount = 0;
        TAtomic SizeInBytes = 0;
    };

}
