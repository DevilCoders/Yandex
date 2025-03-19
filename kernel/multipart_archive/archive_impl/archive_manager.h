#pragma once

#include <kernel/multipart_archive/abstract/part.h>
#include <kernel/multipart_archive/abstract/part_header.h>
#include <library/cpp/logger/global/global.h>

#include <kernel/multipart_archive/protos/archive.pb.h>
#include <library/cpp/protobuf/protofile/protofile.h>

#include "part_header.h"

namespace NRTYArchive {

    template<bool Writable>
    class TPartPolicy : public IArchivePart::IPolicy {
    public:
        TPartPolicy(const IArchivePart::TConstructContext& context)
            : Context(context)
        {}

        IArchivePart::TConstructContext GetContext() const final {
            return Context;
        }

        bool IsWritable() const override {
            return Writable;
        }

    private:
        IArchivePart::TConstructContext Context;
    };

    using TPartMetaSaver = TProtoFileGuard<NRTYArchive::TPartMetaInfo>;

    class IArchiveManager : public TAtomicRefCount<IArchiveManager> {
    public:
        IArchiveManager(const IArchivePart::TConstructContext& constructCtx);
        virtual ~IArchiveManager() {}

        IArchiveManager& SetReadOnly(bool flag = true) {
            ReadOnly = flag;
            return *this;
        }

        bool IsReadOnly() const {
            return ReadOnly;
        }

        const IArchivePart::TConstructContext& GetContext() const {
            return ConstructCtx;
        }

        ui32 GetWritableThreadsCount() const {
            return WritableThreadsCount;
        }

        virtual IPartHeader* CreateHeader(const TFsPath& path, EOpenMode mode) const = 0;
        virtual TAtomicSharedPtr<TPartMetaSaver> CreateMetaSaver(const TFsPath& path) const = 0;

    private:
        void CheckConstructContext() const;

        IArchivePart::TConstructContext ConstructCtx;
        bool ReadOnly = false;

    protected:
        ui32 WritableThreadsCount = 1;
    };

    class TFakeArchiveManager : public IArchiveManager {
    public:
        TFakeArchiveManager(const IArchivePart::TConstructContext& constructCtx)
            : IArchiveManager(constructCtx) {}

        IPartHeader* CreateHeader(const TFsPath& path, EOpenMode mode) const override {
            return new TFakePartHeader(path, mode);
        }

        TAtomicSharedPtr<TPartMetaSaver> CreateMetaSaver(const TFsPath&) const override {
            return nullptr;
        }
    };

    class TArchiveManager : public IArchiveManager {
    public:
        TArchiveManager(const IArchivePart::TConstructContext& constructCtx, bool readonly = false, ui32 writableThreads = 1)
            : IArchiveManager(constructCtx)
        {
            SetReadOnly(readonly);
            WritableThreadsCount = writableThreads;
        }

        IPartHeader* CreateHeader(const TFsPath& path, EOpenMode mode) const override {
            return new TPartHeader(path, mode);
        }

        TAtomicSharedPtr<TPartMetaSaver> CreateMetaSaver(const TFsPath& path) const override {
            return new TPartMetaSaver(path, /*forceOpen*/ false, /*allowUnknownFields*/ true);
        }
    };
}
