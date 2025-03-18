#pragma once

#include <util/generic/buffer.h>
#include <util/generic/utility.h>
#include <util/generic/array_ref.h>
#include <util/system/align.h>
#include <util/system/yassert.h>

#include <library/cpp/offroad/codec/shifted_encoder.h>
#include <library/cpp/offroad/tuple/tuple_output_buffer.h>

#include "key_subtractor.h"

namespace NOffroad {
    template <class KeyData, class Vectorizer, class Subtractor, size_t blockSize, EKeySubtractor keySubtractor = DeltaKeySubtractor>
    class TKeyOutputBuffer {
        enum {
            KeyInfoTupleSize = (keySubtractor == DeltaKeySubtractor ? 2 : 1)
        };

    public:
        using TKey = TString;
        using TKeyRef = TStringBuf;
        using TKeyData = KeyData;

        enum {
            TupleSize = Vectorizer::TupleSize + KeyInfoTupleSize + 1,
            BlockSize = blockSize,
        };

        TKeyOutputBuffer() {
        }

        TKeyOutputBuffer(TKeyOutputBuffer&& other) {
            SwapInternal(other);
        }

        TKeyOutputBuffer& operator=(TKeyOutputBuffer&& other) {
            SwapInternal(other);
            return *this;
        }

        void Reset() {
            KeyInfoBuffer_.Clear();
            DataBuffer_.Reset();
            TextBuffer_.Clear();
            LastKey_.clear();
        }

        void Write(const TKeyRef& key, const TKeyData& data) {
            Y_ASSERT(!IsDone());

            size_t prefixLen;
            size_t suffixLen;
            if (keySubtractor == DeltaKeySubtractor) {
                prefixLen = CommonPrefix(key.data(), key.size(), LastKey_.data(), LastKey_.size());
                suffixLen = key.size() - prefixLen;

                /* Suffix len is never zero if keys are sorted and if there are no equal
                 * keys in index. This is an EOF condition for reader. */
                Y_VERIFY(suffixLen != 0);

                WriteInternal(TStringBuf(key.data() + prefixLen, suffixLen), {prefixLen, suffixLen}, data);
            } else {
                WriteInternal(key, {key.size()}, data);
            }

            /* Copy characters, don't increase refcount. This decreases # of
             * allocations in common use cases. */
            LastKey_.assign(key);
        }

        void Finish() {
            for (size_t i = BlockPosition(); i < BlockSize; i++)
                for (size_t j = 0; j < KeyInfoTupleSize; j++)
                    KeyInfoBuffer_(i, j) = 0;

            DataBuffer_.Finish();
            Y_ASSERT(IsDone());
        }

        bool IsDone() const {
            return DataBuffer_.IsDone();
        }

        template <class Writer>
        void Flush(Writer* writer) {
            static_assert(static_cast<int>(Writer::BlockSize) == static_cast<int>(BlockSize), "Expecting writer with compatible block size.");

            Y_ASSERT(IsDone());

            for (size_t i = 0; i < KeyInfoTupleSize; i++)
                writer->Write(i, KeyInfoBuffer_.Chunk(i));
            DataBuffer_.Flush(AsShiftedEncoder<KeyInfoTupleSize>(writer));

            size_t oldSize = TextBuffer_.Size();
            size_t newSize = AlignUp(oldSize, static_cast<size_t>(BlockSize));
            TextBuffer_.Resize(newSize);
            memset(TextBuffer_.Data() + oldSize, 0, newSize - oldSize);

            ui8* data = reinterpret_cast<ui8*>(TextBuffer_.Data());
            for (size_t i = 0; i < TextBuffer_.Size(); i += BlockSize)
                writer->Write(TupleSize - 1, TArrayRef<ui8>(data + i, BlockSize));

            TextBuffer_.Clear();
        }

        size_t BlockPosition() const {
            return DataBuffer_.BlockPosition();
        }

        const TKey& LastKey() const {
            return LastKey_;
        }

        TKeyData LastData() const {
            return DataBuffer_.Last();
        }

    private:
        void SwapInternal(TKeyOutputBuffer& other) {
            DoSwap(KeyInfoBuffer_, other.KeyInfoBuffer_);
            DoSwap(DataBuffer_, other.DataBuffer_);
            DoSwap(TextBuffer_, other.TextBuffer_);
            DoSwap(LastKey_, other.LastKey_);
        }

        void WriteInternal(const TKeyRef& key, const TArrayRef<const size_t>& info, const TKeyData& data) {
            size_t pos = BlockPosition();

            for (size_t i = 0; i < KeyInfoTupleSize; i++)
                KeyInfoBuffer_(pos, i) = info[i];

            DataBuffer_.WriteHit(data);
            TextBuffer_.Append(key.data(), key.size());
        }

        static size_t CommonPrefix(const char* ptr1, size_t len1, const char* ptr2, size_t len2) {
            size_t len = Min(len1, len2);
            size_t i = 0;
            for (i = 0; i < len && ptr1[i] == ptr2[i]; ++i)
                ;
            return i;
        }

    private:
        TTupleStorage<BlockSize, KeyInfoTupleSize> KeyInfoBuffer_;
        TTupleOutputBuffer<TKeyData, Vectorizer, Subtractor, BlockSize, PlainOldBuffer> DataBuffer_;
        TBuffer TextBuffer_;
        TString LastKey_;
    };

}
