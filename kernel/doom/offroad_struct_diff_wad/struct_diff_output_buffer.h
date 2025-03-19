#pragma once

#include <library/cpp/offroad/tuple/tuple_storage.h>

#include <util/generic/array_ref.h>


namespace NDoom {


template <size_t blockSize, class Key, class KeyVectorizer, class KeySubtractor, class Data>
class TStructDiffOutputBuffer {
public:
    enum {
        BlockSize = blockSize,
        DataSize = sizeof(Data),
        MaskSize = (DataSize + 31) / 32,
        AlignedDataSize = (DataSize + BlockSize - 1) / BlockSize * BlockSize
    };

    using TKey = Key;
    using TData = Data;

    TStructDiffOutputBuffer() {
        Reset();
    }

    void Reset() {
        Data_.clear();
        Filled_ = 0;
        LastKey_.Clear();
        FirstData_.fill(0);
    }

    void Write(const TKey& key, const TData* data) {
        Y_ASSERT(!IsDone());

        NOffroad::TTupleStorage<1, KeyVectorizer::TupleSize> tmpKey;
        KeyVectorizer::Scatter(key, tmpKey.Slice(0));

        KeySubtractor::Differentiate(LastKey_.Slice(0), tmpKey.Slice(0), KeyStorage_.Slice(Filled_));
        LastKey_.Slice(0).Assign(tmpKey.Slice(0));

        const ui8* dataPtr = reinterpret_cast<const ui8*>(data);
        MaskStorage_.Slice(Filled_).Clear();
        if (Filled_ == 0) {
            memcpy(FirstData_.data(), dataPtr, DataSize);
            memcpy(LastData_.data(), dataPtr, DataSize);
        } else {
            for (size_t i = 0; i < DataSize; ++i) {
                if (LastData_[i] != dataPtr[i]) {
                    MaskStorage_(Filled_, i >> 5) |= (ui32(1) << (i & 31));
                    Data_.push_back(dataPtr[i]);
                    LastData_[i] = dataPtr[i];
                }
            }
        }

        ++Filled_;
    }

    bool IsDone() const {
        return Filled_ == BlockSize;
    }

    template <class Writer>
    void Flush(Writer* writer) {
        static_assert(static_cast<ui32>(Writer::BlockSize) == static_cast<ui32>(BlockSize), "Expected same block size");

        if (Filled_ == 0) {
            return;
        }

        Y_ENSURE(!IsZeroSlice(Filled_ - 1), "Your added keys are all zero, add some more keys");

        while (Filled_ < BlockSize) {
            KeyStorage_.Slice(Filled_).Clear();
            MaskStorage_.Slice(Filled_).Clear();
            ++Filled_;
        }

        const size_t newSize = AlignUp(Data_.size(), static_cast<size_t>(BlockSize));
        Data_.resize(newSize, 0);

        for (size_t i = 0; i < AlignedDataSize; i += BlockSize) {
            writer->Write(0, TArrayRef<const ui8>(&FirstData_[i], BlockSize));
        }

        for (size_t i = 0; i < KeyVectorizer::TupleSize; ++i) {
            writer->Write(i + 1, KeyStorage_.Chunk(i));
        }

        for (size_t i = 0; i < MaskSize; ++i) {
            writer->Write(KeyVectorizer::TupleSize + i + 1, MaskStorage_.Chunk(i));
        }

        for (size_t i = 0; i < Data_.size(); i += BlockSize) {
            writer->Write(KeyVectorizer::TupleSize + MaskSize + 1, TArrayRef<const ui8>(&Data_[i], BlockSize));
        }

        Data_.clear();
        Filled_ = 0;
    }

private:
    bool IsZeroSlice(size_t index) const {
        for (size_t i = 0; i < KeyVectorizer::TupleSize; ++i) {
            if (KeyStorage_.Slice(index)[i] != 0) {
                return false;
            }
        }
        return true;
    }

    NOffroad::TTupleStorage<1, KeyVectorizer::TupleSize> LastKey_;
    NOffroad::TTupleStorage<BlockSize, KeyVectorizer::TupleSize> KeyStorage_;
    NOffroad::TTupleStorage<BlockSize, MaskSize> MaskStorage_;
    TVector<ui8> Data_;
    std::array<ui8, AlignedDataSize> FirstData_;
    std::array<ui8, DataSize> LastData_;
    ui32 Filled_ = 0;
};


} // namespace NDoom
