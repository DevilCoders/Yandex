#include "mlock_limiter.h"

#include <util/generic/overloaded.h>
#include <util/stream/format.h>
#include <util/system/mlock.h>


namespace NFileMapper {

namespace {

class TMappedSizeGuard {
public:
    TMappedSizeGuard() = default;

    TMappedSizeGuard(TBytes size, std::atomic<TBytes>* lockedSize)
        : Size_{size}
        , TotalSize_{lockedSize}
    {}

    TMappedSizeGuard(const TMappedSizeGuard&) = delete;

    TMappedSizeGuard& operator=(const TMappedSizeGuard&) = delete;

    TMappedSizeGuard(TMappedSizeGuard&& rhs) noexcept {
        *this = std::move(rhs);
    }

    TMappedSizeGuard& operator=(TMappedSizeGuard&& guard) noexcept {
        if (&guard == this) {
            return *this;
        }
        Release();

        Size_ = std::exchange(guard.Size_, 0);
        TotalSize_ = std::exchange(guard.TotalSize_, nullptr);

        return *this;
    }

    ~TMappedSizeGuard() {
        Release();
    }

private:
    void Release() noexcept {
        if (auto* ptr = std::exchange(TotalSize_, nullptr)) {
            Y_VERIFY(ptr->fetch_sub(Size_) >= Size_);
        }
    }

private:
    TBytes Size_ = 0;
    std::atomic<TBytes>* TotalSize_ = nullptr;
};

} // anonymous namespace

class TFileMapper::TMappedBlobBase
    : public TBlob::TBase
    , public TAtomicRefCount<TMappedBlobBase>
    , private TNonCopyable
{
    using TCounter = TAtomicRefCount<TMappedBlobBase>;

public:
    TMappedBlobBase(TFileMapper* parent, TInode inode, TMappedSizeGuard guard, TBlob blob, EMapMode mode)
        : Parent_{parent}
        , Inode_{inode}
        , Guard_{std::move(guard)}
        , Blob_{std::move(blob)}
        , Mode_{mode}
    {
        if (ShouldLock()) {
            LockMemory(Blob_.data(), Blob_.size());
        }
    }

    ~TMappedBlobBase() {
        if (ShouldLock()) {
            UnlockMemory(Blob_.data(), Blob_.size());
        }
    }

    void Ref() noexcept override {
        return TCounter::Ref();
    }

    void UnRef() noexcept override {
        if (int refCountsLeft = TCounter::RefCount() - 1; refCountsLeft == 1) {
            Parent_->ReleaseFile(Inode_);
        }
        TCounter::UnRef();
    }

    TBlob MakeBlob() {
        return TBlob{Blob_.data(), Blob_.size(), this};
    }

private:
    bool ShouldLock() const {
        return !Blob_.Empty() && Mode_ == EMapMode::Locked;
    }

private:
    TFileMapper* Parent_ = nullptr;
    TInode Inode_ = 0;
    TMappedSizeGuard Guard_;
    TBlob Blob_;
    EMapMode Mode_ = EMapMode::Unlocked;
};

TFileMapper::TFileMapper(TOptions opts)
    : MappedLimit_{opts.MappedLimit}
    , Mapped_{0}
    , OverriddenMapMode_{opts.OverriddenMapMode}
{
}

TFileMapper::~TFileMapper() = default;

TBytes TFileMapper::MappedSize() const {
    return Mapped_.load(std::memory_order_acquire);
}

TBytes TFileMapper::MappedLimit() const {
    return MappedLimit_;
}

TMaybe<TBlob> TFileMapper::TryMapFile(const TString& path, EMapMode mode) {
    TFile file{path, OpenExisting | RdOnly};
    return TryMapFile(file, mode);
}

TMaybe<TBlob> TFileMapper::TryMapFile(const TFsPath& path, EMapMode mode) {
    TFile file{path, OpenExisting | RdOnly};
    return TryMapFile(file, mode);
}

TMaybe<TBlob> TFileMapper::TryMapFile(const TFile& file, EMapMode mode) {
    auto res = TryMapFileImpl(file, mode);
    if (auto* blob = std::get_if<TBlob>(&res)) {
        return std::move(*blob);
    } else {
        return Nothing();
    }
}

TBlob TFileMapper::MapFileOrThrow(const TString& path, EMapMode mode) {
    TFile file{path, OpenExisting | RdOnly};
    return MapFileOrThrow(file, mode);
}

TBlob TFileMapper::MapFileOrThrow(const TFsPath& path, EMapMode mode) {
    TFile file{path, OpenExisting | RdOnly};
    return MapFileOrThrow(file, mode);
}

TBlob TFileMapper::MapFileOrThrow(const TFile& file, EMapMode mode) {
    auto res = TryMapFileImpl(file, mode);
    return std::visit(TOverloaded{
        [](TBlob&& blob) -> TBlob {
            return std::move(blob);
        },
        [this, &file](TFileMapperStats stats) -> TBlob {
            throw TFileMapperLimitReachedError{}
                << "Exceeded mapped files limit " << HumanReadableSize(MappedLimit_, SF_BYTES)
                << ", currenty mapped " << HumanReadableSize(stats.MappedSize, SF_BYTES)
                << ", trying to add " << HumanReadableSize(stats.AdditionalSize, SF_BYTES)
                << " for file " << file.GetName();
        },
    }, std::move(res));
}

std::variant<TBlob, TFileMapperStats> TFileMapper::TryMapFileImpl(const TFile& file, EMapMode mode) {
    auto lockedGuard = TGuard{LockerMutex_};
    ReleaseUnusedFilesImpl();

    TFileStat stat{file};
    const TInode inode = stat.INode;
    if (auto* ptr = FilesByInode_.FindPtr(inode)) {
        return (*ptr)->MakeBlob();
    }
    // After that point we can assume that no other files with the same inode
    // will be locked before we release lockedGuard

    TBlob mappedBlob = TBlob::FromFile(file);
    const TBytes size = mappedBlob.Size();

    // Just sanity check
    Y_ENSURE(size == stat.Size);

    TMappedSizeGuard sizeGuard{size, &Mapped_};
    const TBytes prevMapped = Mapped_.fetch_add(size);
    if (prevMapped + size > MappedLimit_) {
        // NB: Mapped_ value is restored in the dtor of sizeGuard.
        return TFileMapperStats{
            .MappedLimit = MappedLimit_,
            .MappedSize = prevMapped,
            .AdditionalSize = size,
        };
    }

    mode = OverriddenMapMode_.GetOrElse(mode);
    auto base = MakeIntrusive<TMappedBlobBase>(this, inode, std::move(sizeGuard), std::move(mappedBlob), mode);
    auto blob = base->MakeBlob();

    auto [_, emplaced] = FilesByInode_.emplace(inode, std::move(base));
    Y_VERIFY(emplaced);

    return blob;
}

void TFileMapper::ReleaseFile(TInode inode) {
    with_lock (ReleaseQueueMutex_) {
        ReleaseQueue_.push_back(inode);
    }
}

void TFileMapper::ReleaseUnusedFilesImpl() {
    with_lock (LockerMutex_) {
        ReleaseUnusedFiles();
    }
}

void TFileMapper::ReleaseUnusedFiles() {
    TVector<TInode> pending;
    with_lock (ReleaseQueueMutex_) {
        ReleaseQueue_.swap(pending);
    }
    for (TInode inode : pending) {
        auto it = FilesByInode_.find(inode);
        if (it == FilesByInode_.end()) {
            continue;
        }
        if (it->second.RefCount() == 1) {
            FilesByInode_.erase(it);
        }
    }
}

} // namespace NFileMapper
