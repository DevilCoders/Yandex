#pragma once

#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/custom/subtractors.h>
#include <library/cpp/offroad/key/key_input_buffer.h>

namespace NOffroad {
    template <size_t blockSize>
    class TTrieInputBuffer {
        using TKeyBuffer = TKeyInputBuffer<std::nullptr_t, TNullVectorizer, TINSubtractor, blockSize, DeltaKeySubtractor>;
        using TDataBuffer = TKeyInputBuffer<std::nullptr_t, TNullVectorizer, TINSubtractor, blockSize, IdentityKeySubtractor>;

    public:
        using TKey = TString;
        using TKeyRef = TStringBuf;
        using TData = TStringBuf;

        enum {
            TupleSize = TKeyBuffer::TupleSize + TDataBuffer::TupleSize,
            BlockSize = blockSize,
        };

        TTrieInputBuffer() {
        }

        void Reset() {
            KeyBuffer_.Reset();
            DataBuffer_.Reset();
        }

        bool Read(TKeyRef* key, TData* data) {
            Y_ASSERT(!IsDone());

            /* Note that '&' below is intentional, we want buffers to be in sync. */
            std::nullptr_t dummy;
            return KeyBuffer_.Read(key, &dummy) & DataBuffer_.Read(data, &dummy);
        }

        bool Seek(size_t blockPos, const TKeyRef& startKey, const TData& startData) {
            Y_ASSERT(blockPos < BlockSize);

            /* Note that '&' below is intentional, we want buffers to be in sync. */
            return KeyBuffer_.Seek(blockPos, startKey, nullptr) & DataBuffer_.Seek(blockPos, startData, nullptr);
        }

        bool LowerBound(const TKeyRef& prefix, TKeyRef* firstKey, TData* firstData, const TKeyRef& startKey, const TData& startData) {
            Y_ASSERT(prefix >= startKey);

            std::nullptr_t dummy;
            if (!KeyBuffer_.LowerBound(prefix, firstKey, &dummy, startKey, nullptr))
                return false; // TODO: seek in data?

            DataBuffer_.Seek(KeyBuffer_.BlockPosition(), startData, nullptr);
            DataBuffer_.Peek(firstData, &dummy);
            return true;
        }

        bool IsDone() const {
            return DataBuffer_.IsDone();
        }

        bool IsEmpty() const {
            return DataBuffer_.IsEmpty();
        }

        template <class Reader>
        bool Fill(Reader* reader, const TKeyRef& startKey, const TData& startData) {
            static_assert(static_cast<int>(Reader::BlockSize) == static_cast<int>(BlockSize), "Expecting reader with compatible block size.");

            return KeyBuffer_.Fill(reader, startKey, nullptr) &
                   DataBuffer_.Fill(AsShiftedDecoder<TKeyBuffer::TupleSize>(reader), startData, nullptr);
        }

        size_t BlockPosition() const {
            return KeyBuffer_.BlockPosition();
        }

        const TString& LastKey() const {
            return KeyBuffer_.LastKey();
        }

        const TString& LastData() const {
            return DataBuffer_.LastKey();
        }

    private:
        void SwapInternal(TTrieInputBuffer& other) {
            DoSwap(KeyBuffer_, other.KeyBuffer_);
            DoSwap(DataBuffer_, other.DataBuffer_);
        }

    private:
        TKeyBuffer KeyBuffer_;
        TDataBuffer DataBuffer_;
    };

}
