#pragma once

#include <util/system/align.h>
#include <util/system/yassert.h>
#include <util/generic/buffer.h>
#include <util/generic/utility.h>
#include <util/generic/array_ref.h>

#include <library/cpp/offroad/tuple/tuple_input_buffer.h>
#include <library/cpp/offroad/codec/shifted_decoder.h>

#include "key_subtractor.h"

namespace NOffroad {
    template <class KeyData, class Vectorizer, class Subtractor, size_t blockSize, EKeySubtractor keySubtractor = DeltaKeySubtractor>
    class TKeyInputBuffer {
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

        TKeyInputBuffer() {
        }

        TKeyInputBuffer(TKeyInputBuffer&& other) {
            SwapInternal(other);
        }

        TKeyInputBuffer& operator=(TKeyInputBuffer&& other) {
            SwapInternal(other);
            return *this;
        }

        void Reset() {
            DataBuffer_.Reset();
            TextPos_ = 0;
            LastKey_.clear();
        }

        bool Read(TKeyRef* key, TKeyData* data) {
            Y_ASSERT(!IsDone());

            size_t suffixLen = UpdateKeyInternal();
            if (suffixLen == 0)
                return false;

            *key = LastKey_;
            ReadInternal(suffixLen, data);
            return true;
        }

        bool Peek(TKeyRef* key, TKeyData* data) {
            Y_ASSERT(!IsDone());

            size_t suffixLen = UpdateKeyInternal();
            if (suffixLen == 0)
                return false;

            *key = LastKey_;
            DataBuffer_.Peek(data);
            return true;
        }

        bool Seek(size_t blockPos, const TKeyRef& startKey, const TKeyData& startData) {
            Y_ASSERT(blockPos < BlockSize);

            if (BlockPosition() > blockPos)
                Rewind(startKey, startData);

            while (BlockPosition() < blockPos) {
                size_t suffixLen = UpdateKeyInternal();
                if (suffixLen == 0)
                    return false;

                SkipInternal(suffixLen);
            }

            return true;
        }

        bool LowerBound(const TKeyRef& prefix, TKeyRef* firstKey, TKeyData* firstData, const TKeyRef& startKey, const TKeyData& startData) {
            Y_ASSERT(prefix >= startKey);

            if (LastKey_ >= prefix)
                Rewind(startKey, startData);

            while (true) {
                size_t suffixLen = UpdateKeyInternal();
                if (suffixLen == 0)
                    return false;

                if (LastKey_ >= prefix)
                    break;

                SkipInternal(suffixLen);
            }

            *firstKey = LastKey_;
            DataBuffer_.Peek(firstData);
            return true;
        }

        bool IsDone() const {
            return DataBuffer_.IsDone();
        }

        bool IsEmpty() const {
            return DataBuffer_.IsEmpty();
        }

        template <class Reader>
        bool Fill(Reader* reader, const TKeyRef& startKey, const TKeyData& startData) {
            static_assert(static_cast<int>(Reader::BlockSize) == static_cast<int>(BlockSize), "Expecting reader with compatible block size.");

            bool success = true;
            for (size_t i = 0; i < KeyInfoTupleSize; i++)
                success &= reader->Read(i, KeyInfoBuffer_.Chunk(i)) != 0;
            success &= DataBuffer_.Fill(AsShiftedDecoder<KeyInfoTupleSize>(reader), startData);
            if (!success) {
                Y_ASSERT(DataBuffer_.IsDone());
                return false;
            }

            LastKey_ = startKey;

            size_t textSize = 0;
            for (size_t i = 0; i < BlockSize; i++)
                textSize += KeyInfoBuffer_(i, KeyInfoTupleSize - 1); /* Last element -> SuffixLen. */
            textSize = AlignUp(textSize, static_cast<size_t>(BlockSize));

            TextBuffer_.Resize(textSize);
            ui8* data = reinterpret_cast<ui8*>(TextBuffer_.Data());
            for (size_t i = 0; i < TextBuffer_.Size(); i += BlockSize) {
                if (!reader->Read(TupleSize - 1, TArrayRef<ui8>(data + i, BlockSize))) {
                    DataBuffer_.Reset(); /* Make sure IsDone() returns true once we return. */
                    return false;
                }
            }
            TextPos_ = 0;

            return true;
        }

        size_t BlockPosition() const {
            return DataBuffer_.BlockPosition();
        }

        const TString& LastKey() const {
            return LastKey_;
        }

        TKeyData LastData() const {
            return DataBuffer_.Last();
        }

    private:
        void SwapInternal(TKeyInputBuffer& other) {
            DoSwap(KeyInfoBuffer_, other.KeyInfoBuffer_);
            DoSwap(DataBuffer_, other.DataBuffer_);
            DoSwap(TextBuffer_, other.TextBuffer_);
            DoSwap(TextPos_, other.TextPos_);
            DoSwap(LastKey_, other.LastKey_);
        }

        size_t UpdateKeyInternal() {
            size_t pos = BlockPosition();

            if (keySubtractor == DeltaKeySubtractor) {
                size_t prefixLen = KeyInfoBuffer_(pos, 0);
                size_t suffixLen = KeyInfoBuffer_(pos, 1);

                LastKey_.resize(prefixLen);
                LastKey_.append(TextBuffer_.Data() + TextPos_, suffixLen);
                return suffixLen;
            } else {
                size_t len = KeyInfoBuffer_(pos, 0);

                LastKey_.resize(0);
                LastKey_.append(TextBuffer_.Data() + TextPos_, len);
                return len;
            }
        }

        void SkipInternal(size_t suffixLen) {
            DataBuffer_.Skip();
            TextPos_ += suffixLen;
        }

        void ReadInternal(size_t suffixLen, TKeyData* data) {
            DataBuffer_.Read(data);
            TextPos_ += suffixLen;
        }

        void Rewind(const TKeyRef& startKey, const TKeyData& startData) {
            LastKey_.assign(startKey);
            TextPos_ = 0;
            DataBuffer_.Seek(0, startData, TIntegratingSeek());
        }

    private:
        TTupleStorage<BlockSize, KeyInfoTupleSize> KeyInfoBuffer_;
        TTupleInputBuffer<TKeyData, Vectorizer, Subtractor, BlockSize, PlainOldBuffer> DataBuffer_;
        TBuffer TextBuffer_;
        size_t TextPos_ = 0;
        TString LastKey_;
    };

}
