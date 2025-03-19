#pragma once

#include "owner.h"

#include <kernel/multipart_archive/config/config.h>
#include <kernel/multipart_archive/archive_impl/multipart_base.h>
#include <kernel/multipart_archive/statistic/archive_info.h>
#include <kernel/multipart_archive/abstract/optimization_options.h>

#include <util/system/rwlock.h>
#include <utility>

#include <atomic>

namespace NRTYArchive {

    class TMultipartArchive final : public TRefCounted<TMultipartArchive, TAtomicCounter>, public TMultipartImpl<ui32>, public IReadableArchive {
        using TDocId = ui32;
        using TBase = TMultipartImpl<TDocId>;
    public:
        typedef TMap<ui32, ui32> TRemap;
        using TIteratorPtr = typename TBase::TIterator::TPtr;
        using TDocInfo = typename TBase::TDocInfo;

    public:

        TIteratorPtr CreateIterator(bool raw = false);
        TVector<size_t> GetDocIds();
        void Append(const TMultipartArchive& other, const TRemap* remap = nullptr, bool polite = false);
        void Remap(const TVector<ui32>& remap);
        bool OptimizeParts(const TOptimizationOptions& options, const std::atomic<bool>* stopSignal = nullptr);
        void GetInfo(TArchiveInfo& archiveInfo);

        TBlob GetDocument(ui32 docid) const override {
            return ReadDocument(docid);
        }

        static void Repair(const TFsPath& archive, const IArchivePart::TConstructContext& ctx);
        static void Remove(const TFsPath& path);
        static void Rename(const TFsPath& from, const TFsPath& to);
        static bool AllRight(const TFsPath& path);
        static bool IsEmpty(const TFsPath& path);

        static TMultipartArchive* Construct(const TFsPath& path, const TMultipartConfig& config, const NNamedLock::TNamedLockPtr& archiveLock, ui32 docsCount = 0, bool readonly = false);

        ~TMultipartArchive() override {
            DoFlush();
        }

    protected:
        TMultipartArchive(const TFsPath& path, const IArchivePart::TConstructContext& ctx, const TFsPath& offsetsPath);
        TMultipartArchive(const TFsPath& path, IFat* fat, IArchiveManager* manager, const NNamedLock::TNamedLockPtr& archiveLock);

        void InitFromOffsetFile(const TFsPath& offsetsPath);

    private:
        THashMap<ui32, TPosition> GetDocumentsInfo() const;

        TArchivePartThreadSafe::TPtr HardLinkOrCopyPart(ui64 partNum, const TFsPath& newPath, ui64 newPartNum, IArchiveManager* manager, ui32 newDocsCount, IPartCallBack& owner) const;

        void DoClear() override {}
        void DoFlush() override {}
    };


    class TMultipartReadOnlyArchive final
        : public TRefCounted<TMultipartReadOnlyArchive, TAtomicCounter>
        , public TMultipartReadOnlyImpl<ui32>
        , public IReadableArchive
    {
    public:
        static TMultipartReadOnlyArchive* Construct(const TFsPath& path, const TMultipartConfig& config, const NNamedLock::TNamedLockPtr& archiveLock);

    private:
        TMultipartReadOnlyArchive(const TFsPath& path, IFat* fat, IArchiveManager* manager, const NNamedLock::TNamedLockPtr& archiveLock);

    // IReadableArchive implementation
    public:
        TBlob GetDocument(ui32 docid) const override;
    };
}

using TArchiveOwner = NRTYArchive::TMultipartOwner<NRTYArchive::TMultipartArchive>;
using TReadOnlyArchiveOwner = NRTYArchive::TMultipartOwner<NRTYArchive::TMultipartReadOnlyArchive>;
