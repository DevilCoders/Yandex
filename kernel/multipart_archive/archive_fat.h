#pragma once

#include "multipart.h"

#include <library/cpp/threading/hot_swap/hot_swap.h>

#include <util/generic/array_ref.h>
#include <util/system/filemap.h>
#include <util/system/fs.h>

#include <atomic>

namespace NRTYArchive {
    class TFATMultipartReadOnly final : public IFat {
    public:
        TFATMultipartReadOnly(const TFsPath& path, TMemoryMapCommon::EOpenMode mode);
        TPosition Get(size_t docid) const override;
        TPosition Set(size_t /*docid*/, TPosition /*position*/) override { return TPosition::Removed(); }
        ui64 Size() const override;
        void Clear(ui64 /*reserve*/) override {}
        IIterator::TPtr GetIterator() const override {
            return MakeHolder<TSequentialIterator>(GetSnapshot());
        }
        TVector<TPosition> GetSnapshot() const override {
            return {Data, Data + DocsCount};
        }

    private:
        TFileMap FileMap;
        TPosition* Data;
        ui32 DocsCount;
    };

    class TFATMultipart final : public IFat {
    public:
        TFATMultipart(const TFsPath& path, ui32 size, TMemoryMapCommon::EOpenMode mode);
        TPosition Get(size_t docid) const override;
        TPosition Set(size_t docid, TPosition position) override;
        ui64 Size() const override;
        void Clear(ui64 reserve) override;
        IIterator::TPtr GetIterator() const override;
        TVector<TPosition> GetSnapshot() const override;

    private:
        struct TImpl : TThrRefBase {
            TImpl(const TString& path, TMemoryMapCommon::EOpenMode mode);

            TFileMap FileMap;
            TArrayRef<std::atomic<ui64>> Positions;
        };
        TIntrusivePtr<TImpl> ResizeUnlocked(ui32 newSize);
        TIntrusivePtr<TImpl> Grow(ui32 newSize);

        TString Path;
        TMemoryMapCommon::EOpenMode Mode;
        THotSwap<TImpl> Impl;
        TMutex ResizeMutex;
    };

    class TFATBaseArchive final: public IFat {
    public:
        explicit TFATBaseArchive(const TFsPath& path);
        TPosition Get(size_t docid) const override;
        TPosition Set(size_t, TPosition) override;
        ui64 Size() const override;
        void Clear(ui64 reserve) override;
        IIterator::TPtr GetIterator() const override;
        TVector<TPosition> GetSnapshot() const override;

    private:
        TFileMappedArray<ui64> Offsets;
    };
}
