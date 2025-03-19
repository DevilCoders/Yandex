#include "multi_mapper.h"

namespace NDoom {

class TVectorBlobBase : public TBlob::TBase, public TAtomicRefCount<TVectorBlobBase> {
public:
    TVectorBlobBase(TVector<TBlob> blobs)
        : Blobs_(std::move(blobs))
    {
    }

    void Ref() noexcept override {
        TAtomicRefCount<TVectorBlobBase>::Ref();
    }

    void UnRef() noexcept override {
        TAtomicRefCount<TVectorBlobBase>::UnRef();
    }

private:
    TVector<TBlob> Blobs_;
};

TBlob MakeBlobHolder(TVector<TBlob> blobs, TConstArrayRef<char> region) {
    auto base = MakeHolder<TVectorBlobBase>(blobs);
    return TBlob(region.data(), region.size(), base.Release());
}


} // namespace NDoom
