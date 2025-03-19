#pragma once

#include <library/cpp/pop_count/popcount.h>
#include <library/cpp/offroad/tuple/tuple_storage.h>
#include <util/system/align.h>

#include <cstring>

namespace NDoom {


template <size_t blockSize, class Key, class KeyVectorizer, class KeySubtractor, class Data>
class TStructDiffInputBuffer {
public:
    enum {
        BlockSize = blockSize,
        DataSize = sizeof(Data),
        MaskSize = (DataSize + 31) / 32,
        AlignedDataSize = (DataSize + BlockSize - 1) / BlockSize * BlockSize
    };

    using TMaskIndices = std::array<std::array<ui8, 8>, 1 << 8>;

    static constexpr TMaskIndices CalcIndexMasks() {
        TMaskIndices result = {};
        for (ui32 i = 0; i <= Max<ui8>(); ++i) {
            ui8 index = 0;
            for (ui8 j = 0; j < 8; ++j) {
                if ((i >> j) & 1) {
                    result[i][index++] = j + 1;
                }
            }
        }
        return result;
    }

    static constexpr TMaskIndices PreCalcIndexMasks = CalcIndexMasks();

    using TKey = Key;
    using TData = Data;

    TStructDiffInputBuffer() {
        Reset(0);
    }

    TStructDiffInputBuffer(ui32 dataSize) {
        Reset(dataSize);
    }

    void Reset(ui32 dataSize) {
        Y_ENSURE(dataSize <= DataSize);
        MaskSize_ = (dataSize + 31) / 32;
        AlignedDataSize_ = AlignUp(dataSize, static_cast<ui32>(BlockSize));

        LastKey_.Clear();
        BlockPos_ = 0;
        BlockEnd_ = 0;

        if (Y_UNLIKELY(AlignedDataSize_ < AlignedDataSize)) {
            memset(&LastData_[AlignedDataSize_], 0, AlignedDataSize - AlignedDataSize_);
        }
    }

    bool IsDone() const {
        return BlockPos_ == BlockEnd_;
    }

    TKey LastKey() const {
        TKey key;
        KeyVectorizer::Gather(LastKey_.Slice(0), &key);
        return key;
    }

    bool LowerBoundLocal(const TKey& key, TKey* firstKey, const TData** firstData) {
        Y_ASSERT(BlockPos_ == 0);
        Y_ASSERT(!IsDone());

        TKey tmpKey;
        std::array<NOffroad::TTupleStorage<1, KeyVectorizer::TupleSize>, 2> tmpKeyStorage;
        tmpKeyStorage[1].Slice(0).Assign(LastKey_.Slice(0));
        while (BlockPos_ < BlockEnd_) {
            const size_t bit = BlockPos_ & 1;
            KeySubtractor::Integrate(tmpKeyStorage[bit ^ 1].Slice(0), KeyStorage_.Slice(BlockPos_), tmpKeyStorage[bit].Slice(0));
            KeyVectorizer::Gather(tmpKeyStorage[bit].Slice(0), &tmpKey);

            if (tmpKey >= key) {
                size_t curDataPtr = DataPtr_;
                for (size_t i = 0; i < BlockPos_; ++i) {
                    for (size_t j = 0; j < MaskSize; ++j) {
                        curDataPtr += BlockPosBitness_[i][j];
                    }
                }
                DataPtr_ = curDataPtr;
                for (size_t j = 0; j < MaskSize; ++j) {
                    curDataPtr += BlockPosBitness_[BlockPos_][j];
                }
                std::array<ui32, MaskSize> curMask = {{}};
                for (size_t i = BlockPos_; ; --i) {
                    for (size_t j = 0, invJ = MaskSize - 1; j < MaskSize; ++j, --invJ) {
                        const ui32 mask = MaskStorage_(i, invJ);
                        const ui32 newMask = curMask[invJ] | mask;
                        curDataPtr -= BlockPosBitness_[i][invJ];
                        if (newMask != curMask[invJ]) {
                            const ui32 diffMask = newMask ^ curMask[invJ];
                            curMask[invJ] = newMask;
                            const ui8* bytes = reinterpret_cast<const ui8*>(&mask);
                            ui8 it;
                            const ui8* curIndexMasks;
#define FILL_DATA(var, offset)                              \
    curIndexMasks = PreCalcIndexMasks[var].data();          \
    it = 0;                                                 \
    while (it < 8 && curIndexMasks[it] != 0) {              \
        const ui8 pos = curIndexMasks[it++] + offset;       \
        if ((diffMask >> pos) & 1) {                        \
            LastData_[invJ * 32 + pos] = Data_[tmpDataPtr]; \
        }                                                   \
        ++tmpDataPtr;                                       \
    }


                            size_t tmpDataPtr = curDataPtr;
                            FILL_DATA(bytes[0], -1);
                            FILL_DATA(bytes[1], 7);
                            FILL_DATA(bytes[2], 15);
                            FILL_DATA(bytes[3], 23);
#undef FILL_DATA
                        }
                    }
                    if (i == 0) {
                        break;
                    }
                }

                LastKey_.Slice(0).Assign(tmpKeyStorage[bit ^ 1].Slice(0));
                *firstKey = std::move(tmpKey);
                *firstData = reinterpret_cast<const TData*>(LastData_.data());
                return true;
            }

            ++BlockPos_;
        }

        return false;
    }

    template <class Reader>
    bool FillBlock(Reader* reader, const TKey& lastKey) {
        static_assert(static_cast<int>(Reader::BlockSize) == static_cast<int>(BlockSize), "Expected same block size");

        KeyVectorizer::Scatter(lastKey, LastKey_.Slice(0));

        for (size_t i = 0; i < AlignedDataSize_; i += BlockSize) {
            if (reader->Read(0, TArrayRef<ui8>(&LastData_[i], BlockSize)) == 0) {
                return false;
            }
        }

        for (size_t i = 0; i < KeyVectorizer::TupleSize; ++i) {
            if (reader->Read(i + 1, KeyStorage_.Chunk(i)) == 0) {
                return false;
            }
        }

        size_t dataSize = 0;

        for (size_t i = 0; i < MaskSize; ++i) {
            if (Y_UNLIKELY(i >= MaskSize_)) {
                memset(MaskStorage_.Chunk(i).data(), 0, BlockSize * sizeof(ui32));
                continue;
            }
            if (reader->Read(KeyVectorizer::TupleSize + i + 1, MaskStorage_.Chunk(i)) == 0) {
                return false;
            }
            for (size_t j = 0; j < BlockSize; ++j) {
                BlockPosBitness_[j][i] = PopCount(MaskStorage_(j, i));
                dataSize += BlockPosBitness_[j][i];
            }
        }

        dataSize = AlignUp(dataSize, static_cast<size_t>(BlockSize));

        DataPtr_ = 0;
        Data_.resize(dataSize);

        for (size_t i = 0; i < Data_.size(); i += BlockSize) {
            if (reader->Read(KeyVectorizer::TupleSize + MaskSize_ + 1, TArrayRef<ui8>(&Data_[i], BlockSize)) == 0) {
                return false;
            }
        }

        BlockPos_ = 0;
        BlockEnd_ = BlockSize;

        while (BlockEnd_ > 0 && IsZeroSlice(BlockEnd_ - 1)) {
            --BlockEnd_;
        }

        return BlockEnd_ > 0;
    }

    void Read(TKey* key, const TData** data) {
        Y_ASSERT(!IsDone());

        KeySubtractor::Integrate(LastKey_.Slice(0), KeyStorage_.Slice(BlockPos_), LastKey_.Slice(0));
        KeyVectorizer::Gather(LastKey_.Slice(0), key);
        for (size_t i = 0; i < DataSize; ++i) {
            if ((MaskStorage_(BlockPos_, i >> 5) >> (i & 31)) & 1) {
                LastData_[i] = Data_[DataPtr_++];
            }
        }

        *data = reinterpret_cast<const TData*>(LastData_.data());

        ++BlockPos_;
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


    ui32 MaskSize_ = 0;
    ui32 AlignedDataSize_ = 0;

    ui32 BlockPos_ = 0;
    ui32 BlockEnd_ = 0;
    size_t DataPtr_ = 0;
    NOffroad::TTupleStorage<1, KeyVectorizer::TupleSize> LastKey_;
    NOffroad::TTupleStorage<BlockSize, KeyVectorizer::TupleSize> KeyStorage_;
    NOffroad::TTupleStorage<BlockSize, MaskSize> MaskStorage_;
    std::array<std::array<ui32, MaskSize>, BlockSize> BlockPosBitness_;
    TVector<ui8> Data_;
    std::array<ui8, AlignedDataSize> LastData_;
};


} // namespace NDoom
