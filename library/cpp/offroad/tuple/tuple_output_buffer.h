#pragma once

#include "tuple_storage.h"
#include <library/cpp/offroad/codec/buffer_type.h>
#include <util/string/cast.h>

namespace NOffroad {
    template <class Data, class Vectorizer, class Subtractor, size_t blockSize, EBufferType type = AutoEofBuffer>
    class TTupleOutputBuffer {
        static_assert(
            /* Need these static_casts to make GCC happy. */
            static_cast<int>(Vectorizer::TupleSize) == static_cast<int>(Subtractor::TupleSize) || Subtractor::TupleSize == -1,
            "Expecting compatible vectorizer & subtractor.");

    public:
        using THit = Data;

        enum {
            TupleSize = Vectorizer::TupleSize,
            BlockSize = blockSize
        };

        TTupleOutputBuffer() {
            Reset();
        }

        void Reset() {
            AtSeekPoint_ = true;
            LastData_.Clear();
            BlockPos_ = 0;
            ZeroCount_ = 0;
        }

        void WriteHit(const THit& data) {
            Y_ASSERT(!IsDone());

            TTupleStorage<1, TupleSize> tmp;
            Vectorizer::Scatter(data, tmp.Slice(0));
            Subtractor::Differentiate(LastData_.Slice(0), tmp.Slice(0), Storage_.Slice(BlockPos_));

            ValidateDelta(tmp.Slice(0));

            LastData_.Slice(0).Assign(tmp.Slice(0));
            BlockPos_++;
            AtSeekPoint_ = false;
        }

        void WriteSeekPoint() {
            AtSeekPoint_ = true;
            LastData_.Clear();
        }

        void Finish() {
            if (IsEmpty())
                return; /* Nothing here, so nothing to clear. */

            ZeroCount_ = BlockSize - BlockPos_;
            while (!IsDone())
                Storage_.Slice(BlockPos_++).Clear();
        }

        bool IsDone() const {
            return BlockPos_ == BlockSize;
        }

        bool IsEmpty() const {
            return BlockPos_ == 0;
        }

        template <class Writer>
        void Flush(Writer* writer) {
            static_assert(static_cast<int>(BlockSize) % static_cast<int>(Writer::BlockSize) == 0, "Expecting writer with compatible block size.");

            Y_ASSERT(IsDone() || IsEmpty());

            if (IsEmpty())
                return; /* Nothing to flush. */

            if (static_cast<int>(BlockSize) == static_cast<int>(Writer::BlockSize)) {
                for (size_t i = 0; i < TupleSize; i++)
                    writer->Write(i, Storage_.Chunk(i));
            } else {
                size_t maxShift = BlockSize - ZeroCount_;
                for (size_t shift = 0; shift < maxShift; shift += Writer::BlockSize)
                    for (size_t i = 0; i < TupleSize; i++)
                        writer->Write(i, Storage_.Chunk(i, shift, Writer::BlockSize));
            }

            BlockPos_ = 0;
            ZeroCount_ = 0;
        }

        size_t BlockPosition() const {
            return BlockPos_;
        }

        THit Last() const {
            THit result;
            Vectorizer::Gather(LastData_.Slice(0), &result);
            return result;
        }

    private:
        template <class Slice>
        void ValidateDelta(const Slice& next) {
            /* No need to validate anything if we don't want to have EOF
             * autodetection on the other end. */
            if (type != AutoEofBuffer)
                return;

            if (TupleSize == 0)
                return;

            auto&& delta = Storage_.Slice(BlockPos_);
            if (delta.Mask() != 0)
                return;

            /* Zero as first element is entirely possible, and thus allowed.
             * It, however, will be lost if that's the only element in a sequence,
             * but we don't really care. */
            if (AtSeekPoint_ && next.Mask() == 0)
                return;

            auto&& value = LastData_.Slice(0);

            /* Note: if you're getting an exception here, it might be the case that
             * you should be using PlainOldBuffer and not AutoEofBuffer as buffer type. */
            ythrow yexception() << "Stored data tuples must be unique "
                                << "(last data = " << SliceToString(value) << ", new data = " << SliceToString(next) << ", delta = " << SliceToString(delta) << ")";
        }

        template <class Slice>
        TString SliceToString(const Slice& slice) {
            TString s = "[";
            for (size_t i = 0; i < TupleSize; i++) {
                if (i != 0)
                    s += ".";
                s += ToString(slice[i]);
            }
            s += "]";
            return s;
        }

    private:
        size_t BlockPos_ = 0;
        size_t ZeroCount_ = 0;
        bool AtSeekPoint_ = true;
        TTupleStorage<1, TupleSize> LastData_;
        TTupleStorage<BlockSize, TupleSize> Storage_;
    };

}
