#pragma once

#include <library/cpp/offroad/codec/decoder_64.h>
#include <library/cpp/offroad/codec/interleaved_decoder.h>
#include <library/cpp/offroad/offset/data_offset.h>

#include "key_table.h"
#include "key_input_buffer.h"

namespace NOffroad {
    template <class KeyData, class Vectorizer, class Subtractor, EKeySubtractor keySubtractor = DeltaKeySubtractor>
    class TKeyReader {
        using TInputBuffer = TKeyInputBuffer<KeyData, Vectorizer, Subtractor, 64, keySubtractor>;
        using TDecoder = TInterleavedDecoder<TInputBuffer::TupleSize, TDecoder64>;

    public:
        using TKey = TString;
        using TKeyRef = TStringBuf;
        using TKeyData = KeyData;
        using TTable = TKeyTable<typename TDecoder::TTable>;
        using TModel = typename TTable::TModel;
        using TPosition = TDataOffset;

        enum {
            BlockSize = 64,
        };

        TKeyReader() {
        }

        TKeyReader(const TTable* table, const TArrayRef<const char>& source) {
            Reset(table, source);
        }

        TKeyReader(const TTable* table, const TBlob& blob) {
            Reset(table, blob);
        }

        TKeyReader(const TKeyReader&) = delete;

        TKeyReader(TKeyReader&& other) {
            *this = std::move(other);
        }

        TKeyReader& operator=(const TKeyReader&) = delete;

        TKeyReader& operator=(TKeyReader&& other)  {
            SwapInternal(other);
            return *this;
        }

        void Reset(const TTable* table, const TBlob& blob) {
            Table_ = table;
            Input_.Reset(blob);
            Decoder_.Reset(&table->Base(), &Input_);
            StreamPosition_ = 0;
            Buffer_.Reset();
        }

        void Reset(const TTable* table, const TArrayRef<const char>& source) {
            Reset(table, TBlob::NoCopy(source.data(), source.size()));
        }

        void Restart() {
            Input_.Seek(0);
            StreamPosition_ = 0;
            Buffer_.Reset();
        }

        bool ReadKey(TKeyRef* key, TKeyData* data) {
            if (Buffer_.IsDone())
                if (!FillBuffer(Buffer_.LastKey(), Buffer_.LastData()))
                    return false;

            return Buffer_.Read(key, data);
        }

        bool Seek(TDataOffset position, const TKeyRef& startKey, const TKeyData& startData) {
            ui64 streamPosition = position.Offset();
            size_t blockPosition = position.Index();

            if (streamPosition != StreamPosition_ || Y_UNLIKELY(Buffer_.IsEmpty())) {
                if (!Input_.Seek(streamPosition))
                    return false;

                if (!FillBuffer(startKey, startData))
                    return false;
            }

            return Buffer_.Seek(blockPosition, startKey, startData);
        }

        bool LowerBoundLocal(const TKeyRef& prefix, TKeyRef* firstKey, TKeyData* firstData, const TKeyRef& startKey, const TKeyData& startData) {
            return Buffer_.LowerBound(prefix, firstKey, firstData, startKey, startData);
        }

        /**
         * @returns                         Current input position.
         */
        TDataOffset Position() const {
            if (Y_UNLIKELY(Buffer_.IsDone())) {
                return TDataOffset(Input_.Position(), 0);
            } else {
                return TDataOffset(StreamPosition_, Buffer_.BlockPosition());
            }
        }

        const TTable* Table() const {
            return Table_;
        }

        const TKey& LastKey() const {
            return Buffer_.LastKey();
        }

        TKeyData LastData() const {
            return Buffer_.LastData();
        }

    private:
        void SwapInternal(TKeyReader& other) {
            DoSwap(Table_, other.Table_);
            DoSwap(Input_, other.Input_);
            DoSwap(StreamPosition_, other.StreamPosition_);
            DoSwap(Decoder_, other.Decoder_);
            DoSwap(Buffer_, other.Buffer_);
            Decoder_.Reset(Decoder_.Table(), &Input_);
            other.Decoder_.Reset(other.Decoder_.Table(), &other.Input_);
        }

        bool FillBuffer(const TKeyRef& startKey, const TKeyData& startData) {
            StreamPosition_ = Input_.Position();
            return Buffer_.Fill(&Decoder_, startKey, startData);
        }

    private:
        const TTable* Table_ = nullptr;
        TVecInput Input_;
        ui64 StreamPosition_ = 0;
        TDecoder Decoder_;
        TInputBuffer Buffer_;
    };

}
