#include "archive_fat.h"

#include <library/cpp/logger/global/global.h>
#include <util/folder/path.h>

namespace NRTYArchive {

    //TFATMultipartReadOnly
    TFATMultipartReadOnly::TFATMultipartReadOnly(const TFsPath& path, TMemoryMapCommon::EOpenMode mode)
        : FileMap(path, mode)
    {
        Data = static_cast<TPosition*>(FileMap.Map(0, FileMap.Length()).MappedData());
        DocsCount = FileMap.Length() / sizeof(TPosition);
    }

    TPosition TFATMultipartReadOnly::Get(size_t docid) const  {
        if (docid >= DocsCount) {
            DEBUG_LOG << "position is incorrect: " << docid << "/" << DocsCount << Endl;
            return TPosition::Removed();
        }
        return Data[docid];
    }

    ui64 TFATMultipartReadOnly::Size() const  {
        return DocsCount;
    }

    TFATMultipart::TImpl::TImpl(const TString& path, TMemoryMapCommon::EOpenMode mode)
        : FileMap(path, mode)
        , Positions(
            reinterpret_cast<std::atomic<ui64>*>(FileMap.Map(0, FileMap.Length()).MappedData()),
            FileMap.Length() / sizeof(TPosition))
    {
        Y_VERIFY(Positions.empty() || reinterpret_cast<intptr_t>(Positions.data()) % sizeof(ui64) == 0);
    }

    //TFATMultipart
    TFATMultipart::TFATMultipart(const TFsPath& path, ui32 size, TMemoryMapCommon::EOpenMode mode)
        : Path(path.GetPath())
        , Mode(mode)
    {
        const bool exists = path.Exists();
        Y_ENSURE(exists || Mode != TMemoryMapCommon::oRdOnly, "there is no " << path.GetPath(););
        if (!exists) {
            path.Touch();
        }
        Impl.AtomicStore(MakeIntrusive<TImpl>(path, Mode));
        if (!exists && size > 0) {
            Grow(size);
        }
    }

    TPosition TFATMultipart::Get(size_t docid) const  {
        auto impl = Impl.AtomicLoad();
        if (docid >= impl->Positions.size()) {
            DEBUG_LOG << "position is incorrect: " << docid << "/" << impl->Positions.size() << Endl;
            return TPosition::Removed();
        }
        return TPosition::FromRepr(impl->Positions[docid].load(std::memory_order_relaxed));
    }

    TPosition TFATMultipart::Set(size_t docid, TPosition position)  {
        auto impl = Impl.AtomicLoad();
        if (docid >= impl->Positions.size()) {
            impl = Grow(docid + 1);
        }
        return TPosition::FromRepr(
                impl->Positions[docid].exchange(position.Repr(), std::memory_order_relaxed));
    }

    ui64 TFATMultipart::Size() const  {
        return Impl.AtomicLoad()->Positions.size();
    }

    void TFATMultipart::Clear(ui64 reserve) {
        auto guard = Guard(ResizeMutex);
        auto impl = ResizeUnlocked(reserve);
        for (auto& pos: impl->Positions) {
            pos.store(TPosition::Removed().Repr(), std::memory_order_relaxed);
        }
    }

    TFATMultipart::IIterator::TPtr TFATMultipart::GetIterator() const {
        return MakeHolder<TSequentialIterator>(GetSnapshot());
    }

    TVector<TPosition> TFATMultipart::GetSnapshot() const {
        auto impl = Impl.AtomicLoad();
        TVector<TPosition> result(impl->Positions.size());
        for (size_t i = 0; i != impl->Positions.size(); ++i) {
            result[i] = TPosition::FromRepr(impl->Positions[i].load(std::memory_order_relaxed));
        }
        return result;
    }

    // When shrinking size, there should not be any outstanding readers.
    TIntrusivePtr<TFATMultipart::TImpl> TFATMultipart::ResizeUnlocked(ui32 newSize)  {
        auto impl = Impl.AtomicLoad();
        const auto oldSize = impl->Positions.size();
        if (oldSize == newSize) {
            return impl;
        }
        TFile{Path, RdWr}.Resize(newSize * sizeof(TPosition));
        impl = MakeIntrusive<TImpl>(Path, Mode);

        for (size_t i = oldSize; i < newSize; ++i) {
            impl->Positions[i].store(TPosition::Removed().Repr(), std::memory_order_relaxed);
        }

        Impl.AtomicStore(impl);

        return impl;
    }

    TIntrusivePtr<TFATMultipart::TImpl> TFATMultipart::Grow(ui32 newSize)  {
        auto guard = Guard(ResizeMutex);
        auto impl = Impl.AtomicLoad();
        return impl->Positions.size() >= newSize ? impl : ResizeUnlocked(newSize);
    }

    // TFATBaseArchive
    TFATBaseArchive::TFATBaseArchive(const TFsPath& path) {
        Y_ENSURE(path.Exists(), "there is no " << path.GetPath());
        Offsets.Init(path.GetPath().data());
    }

   TPosition TFATBaseArchive::Get(size_t docid) const  {
        if (docid >= Offsets.size() || Offsets[docid] == Max<ui64>())
            return TPosition::Removed();

        return TPosition(Offsets[docid], 0);
    }

    TPosition TFATBaseArchive::Set(size_t, TPosition)  {
        FAIL_LOG("Operation not permitted");
        return TPosition::Removed();
    }

    ui64 TFATBaseArchive::Size() const  {
        return Offsets.size();
    }

    void TFATBaseArchive::Clear(ui64)  {
        FAIL_LOG("Operation not permitted");
    }

    TFATBaseArchive::IIterator::TPtr TFATBaseArchive::GetIterator() const {
        return MakeHolder<TSequentialIterator>(GetSnapshot());
    }

    TVector<TPosition> TFATBaseArchive::GetSnapshot() const  {
        TVector<TPosition> result(Reserve(Size()));
        for (auto offset: Offsets) {
            result.push_back(offset == Max<ui64>() ? TPosition::Removed() : TPosition(offset, 0));
        }
        return result;
    }
}
