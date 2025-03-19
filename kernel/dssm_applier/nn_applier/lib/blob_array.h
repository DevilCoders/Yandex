#pragma once

#include <kernel/dssm_applier/nn_applier/lib/saveload_utils.h>

#include <util/memory/blob.h>
#include <util/generic/array_ref.h>
#include <util/ysaveload.h>

namespace NNeuralNetApplier {

template <class T>
class TBlobArray : public TArrayRef<const T> {
private:
    using TBase = TArrayRef<const T>;
    TBlob DataHolder;

    explicit TBlobArray(TArrayRef<const T> arrayRef)
        : TBase(arrayRef)
        , DataHolder(TBlob::NoCopy(arrayRef.data(), arrayRef.size_bytes()))
    {
    }

public:
    TBlobArray() = default;

    static TBlobArray<T> NoCopy(TArrayRef<const T> arrayRef) {
        return TBlobArray<T>(arrayRef);
    }

    size_t Load(const TBlob& data) {
        TBlob curBlob = data;
        size_t dataLen = ReadSize(curBlob);
        DataHolder = curBlob.SubBlob(0, dataLen);

        const T* regionPtr = reinterpret_cast<const T*>(DataHolder.AsCharPtr());
        size_t regionLen = (DataHolder.Size()) / sizeof(T);

        TBase::operator = (TBase(regionPtr, regionLen));
        return sizeof(size_t) + DataHolder.Size();
    }

    void Save(IOutputStream* s) const {
        ::Save(s, DataHolder.Size());
        ::SaveArray(s, DataHolder.AsCharPtr(), DataHolder.Size());
    }
};

}
