#pragma once

#include "flat_blob_storage.h"

#include <util/memory/blob.h>
#include <util/generic/vector.h>

namespace NDoom {

// symmetric check hints the compiler that IsSame is commutative
// https://stackoverflow.com/questions/58509147/why-does-same-as-concept-check-type-equality-twice
template <typename U, typename V>
concept IsSame = std::is_same_v<U, V> && std::is_same_v<V, U>;

template <typename T>
concept TFileLoader = requires(T t) {
    { t.GetFileBlob(TString{}, bool{}) } -> IsSame<TBlob>; // TODO: replace with std::convertible_to
};

template <TFileLoader TLoader>
class TFlatStorageWithLoader: public IFlatBlobStorage {
public:
    TFlatStorageWithLoader() = default;

    template <typename... Args>
    TFlatStorageWithLoader(Args&&... args) {
        Reset(std::forward<Args>(args)...);
    }

    void Reset(TConstArrayRef<TString> files, bool lockMemory = false) {
        Reset(TConstArrayRef<TConstArrayRef<TString>>{files}, lockMemory);
    }

    void Reset(TConstArrayRef<TConstArrayRef<TString>> files, bool lockMemory = false) {
        Maps_ = TVector<TVector<TBlob>>(files.size());
        for (size_t i = 0; i < files.size(); ++i) {
            Maps_[i] = TVector<TBlob>(files[i].size());
            for (size_t j = 0; j < files[i].size(); ++j) {
                Maps_[i][j] = Loader_.GetFileBlob(files[i][j], lockMemory);
            }
        }
    }

    template <typename U, typename V, typename... Args>
    void Reset(U&& u, V&& v, Args&&... args) {
        Loader_.Reset(std::forward<Args>(args)...);
        Reset(std::forward<U>(u), std::forward<V>(v));
    }

    using IFlatBlobStorage::Read;

    void Read(
        TConstArrayRef<ui32> chunks,
        TConstArrayRef<ui32> parts,
        TConstArrayRef<ui64> offsets,
        TConstArrayRef<ui64> sizes,
        TArrayRef<TBlob> blobs) const override
    {
        for (size_t i = 0; i < offsets.size(); ++i) {
            blobs[i] = TBlob::NoCopy(Maps_[chunks[i]][parts[i]].AsCharPtr() + offsets[i], sizes[i]);
        }
    }

    ui32 Chunks() const override {
        return Maps_.size();
    }

private:
    TLoader Loader_;
    TVector<TVector<TBlob>> Maps_;
};

struct TMappedFileLoader {
    TBlob GetFileBlob(const TString& path, bool lockMemory) const {
        return lockMemory ? TBlob::LockedFromFile(path) : TBlob::FromFile(path);
    }
};

using TMappedFlatStorage = TFlatStorageWithLoader<TMappedFileLoader>;

} // namespace NDoom
