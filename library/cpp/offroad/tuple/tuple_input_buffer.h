#pragma once

#include "tuple_storage.h"
#include "seek_mode.h"

#include <library/cpp/offroad/codec/buffer_type.h>

#include <util/system/types.h>
#include <util/generic/algorithm.h>
#include <util/generic/xrange.h>

#include <array>

namespace NOffroad {
    template <class Data, class Vectorizer, class Subtractor, size_t blockSize, EBufferType type = AutoEofBuffer>
    class TTupleInputBuffer {
        static_assert(
            /* Need these static_casts to make GCC happy. */
            static_cast<int>(Vectorizer::TupleSize) == static_cast<int>(Subtractor::TupleSize) || Subtractor::TupleSize == -1,
            "Expecting compatible vectorizer & subtractor.");

    public:
        using THit = Data;

        enum {
            TupleSize = Vectorizer::TupleSize,
            PrefixSize = Subtractor::PrefixSize == -1 ? static_cast<int>(TupleSize) : static_cast<int>(Subtractor::PrefixSize),
            BlockSize = blockSize,
        };

        TTupleInputBuffer() {
            Reset();
        }

        void Reset() {
            LastData_.Clear();
            Invalidate();
        }

        Y_FORCE_INLINE void Read(THit* data) {
            Y_ASSERT(!IsDone());

            Subtractor::Integrate(LastData_.Slice(0), Storage_.Slice(BlockPos_++), LastData_.Slice(0));
            Vectorizer::Gather(LastData_.Slice(0), data);
        }

        /**
         *  @returns                    False iff consumer returns false.
         */
        template <class Consumer>
        Y_FORCE_INLINE bool ReadHits(const Consumer& consumer) {
            THit data;
            TTupleStorage<1, TupleSize> tmp;
            LastData_.CopyTo(tmp);
            while (!IsDone()) {
                Subtractor::Integrate(LastData_.Slice(0), Storage_.Slice(BlockPos_), tmp.Slice(0));
                Vectorizer::Gather(tmp.Slice(0), &data);
                if (!consumer(data)) {
                    return false;
                }
                tmp.CopyTo(LastData_);
                ++BlockPos_;
            }
            return true;
        }

        Y_FORCE_INLINE void Peek(THit* data) const {
            Y_ASSERT(!IsDone());

            TTupleStorage<1, TupleSize> tmp;
            LastData_.CopyTo(tmp);
            Subtractor::Integrate(LastData_.Slice(0), Storage_.Slice(BlockPos_), tmp.Slice(0));
            Vectorizer::Gather(tmp.Slice(0), data);
        }

        Y_FORCE_INLINE void Skip() {
            Y_ASSERT(!IsDone());

            Subtractor::Integrate(LastData_.Slice(0), Storage_.Slice(BlockPos_++), LastData_.Slice(0));
        }

        Y_FORCE_INLINE bool IsDone() const {
            return BlockPos_ == BlockEnd_;
        }

        Y_FORCE_INLINE bool IsEmpty() const {
            return BlockEnd_ == 1;
        }

        template <class SeekMode>
        bool Seek(size_t blockPos, const THit& startData, SeekMode mode) {
            blockPos++; /* Adjust for how internal position is stored. */

            if (blockPos > BlockEnd_)
                return false; /* Note that we use '>' here so that seeks-to-end are allowed. */

            if (mode == SeekPointSeek) {
                BlockPos_ = blockPos;
                LastData_.Clear();
            } else {
                Vectorizer::Scatter(startData, LastData_.Slice(0));

                BlockPos_ = 1;
                while (BlockPos_ < blockPos)
                    Skip();
            }

            return true;
        }

        bool LowerBound(const THit& prefix, THit* first, size_t from, size_t to, bool isFirstBlock) {
            static_assert(PrefixSize > 0, "Cannot perform a lower bound when subtractor-provided prefix is empty.");
            if (IsEmpty()) {
                return false;
            }

            ++from;
            ++to;
            Y_ASSERT(from <= to);

            if (to > BlockEnd_) {
                to = BlockEnd_;
            }

            TTupleStorage<1, TupleSize> tmp;
            Vectorizer::Scatter(prefix, tmp.Slice(0));

            auto less = [&](size_t l, void*) {
                return Storage_.Slice(l).template CompareLess<PrefixSize>(tmp.Slice(0));
            };

            auto range = xrange<size_t>((isFirstBlock ? from : from - 1), to);
            size_t index = *::LowerBound(range.begin(), range.end(), nullptr, less);

            if (index >= from && index < to) {
                BlockPos_ = index;
                Vectorizer::Gather(Storage_.Slice(index), first);
                LastData_.Clear();
                return true;
            } else {
                return false;
            }
        }

        template <class Reader>
        bool Fill(Reader* reader, const THit& startData, size_t integrationShift = 0) {
            static_assert(static_cast<int>(BlockSize) % static_cast<int>(Reader::BlockSize) == 0, "Expecting reader with compatible block size.");

            if (TupleSize == 0) {
                BlockPos_ = 1;
                BlockEnd_ = BlockSize + 1;
                return true;
            }

            /* Note that we use `success &=` below so that we don't mess up the states
             * of the underlying streams in case one of the reads fails. */

            bool success = true;
            size_t blockLast;
            if (static_cast<int>(BlockSize) == static_cast<int>(Reader::BlockSize)) {
                for (size_t i = 0; i < TupleSize; i++)
                    success &= reader->Read(i, Storage_.Chunk(i, 1)) != 0;
                blockLast = BlockSize;
            } else {
                size_t shift = 0;
                for (; shift < BlockSize; shift += Reader::BlockSize) {
                    for (size_t i = 0; i < TupleSize; i++)
                        success &= reader->Read(i, Storage_.Chunk(i, 1 + shift, Reader::BlockSize)) != 0;
                    if (!success)
                        break;
                }
                if (shift > 0)
                    success = true;
                blockLast = shift;
            }

            if (!success) {
                Invalidate();
                return false;
            }

            Vectorizer::Scatter(startData, Storage_.Slice(integrationShift));
            Vectorizer::Scatter(startData, LastData_.Slice(0));

            /* Might be the final block. Check it. */
            if (type == AutoEofBuffer && Storage_.Slice(blockLast).Mask() == 0) {
                while (Storage_.Slice(blockLast).Mask() == 0) {
                    blockLast--;

                    if (blockLast == 0) {
                        /* We _might_ end up with a buffer filled with all zeros.
                         * This happens when last m128 block is only partially filled.
                         * So we can read from it, while in fact the stream has already ended. */
                        Invalidate();
                        return false;
                    }
                }
            }

            BlockEnd_ = blockLast + 1;
            BlockPos_ = 1;
            if (integrationShift > 0) {
                Subtractor::Integrate(Storage_.Shifted(integrationShift));
            } else {
                Subtractor::Integrate(Storage_);
            }
            return true;
        }

        Y_FORCE_INLINE size_t BlockPosition() const {
            return BlockPos_ - 1;
        }

        Y_FORCE_INLINE THit Last() const {
            THit result;
            Vectorizer::Gather(LastData_.Slice(0), &result);
            return result;
        }

    private:
        void Invalidate() {
            BlockPos_ = 1;
            BlockEnd_ = 1;
        }

    private:
        size_t BlockPos_ = BlockSize + 1;
        size_t BlockEnd_ = BlockSize + 1;
        TTupleStorage<1, TupleSize> LastData_;
        TTupleStorage<BlockSize + 1, TupleSize> Storage_; /* +1 here is needed for efficient in-block seeks. */
    };

}
