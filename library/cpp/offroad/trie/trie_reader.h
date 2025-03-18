#pragma once

#include <util/generic/ptr.h>

#include <library/cpp/offroad/codec/decoder_64.h>
#include <library/cpp/offroad/codec/interleaved_decoder.h>
#include <library/cpp/offroad/offset/data_offset.h>

#include "trie_input_buffer.h"

namespace NOffroad {
    class TTrieReader {
        using TInputBuffer = TTrieInputBuffer<64>;
        using TDecoder = TInterleavedDecoder<TInputBuffer::TupleSize, TDecoder64>;

    public:
        using TKey = TString;
        using TKeyRef = TStringBuf;
        using TData = TStringBuf;
        using TTable = typename TDecoder::TTable;
        using TModel = typename TTable::TModel;

        enum {
            BlockSize = 64,
        };

        TTrieReader() {
        }

        TTrieReader(const TTable* table, const TArrayRef<const char>& source) {
            Reset(table, source);
        }

        TTrieReader(const TTrieReader&) = delete;

        TTrieReader(TTrieReader&& other) {
            SwapInternal(other);
        }

        void Reset(const TTable* table, const TArrayRef<const char>& source) {
            Table_ = table;
            Input_.Reset(source);
            Decoder_.Reset(table, &Input_);
            StreamPosition_ = 0;
            Buffer_.Reset();
        }

        void Restart() {
            Input_.Seek(0);
            StreamPosition_ = 0;
            Buffer_.Reset();
        }

        bool Read(TKeyRef* key, TData* data) {
            if (Buffer_.IsDone())
                if (!FillBuffer(Buffer_.LastKey(), Buffer_.LastData()))
                    return false;

            return Buffer_.Read(key, data);
        }

        bool Seek(TDataOffset position, const TKeyRef& startKey, const TData& startData) {
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

        bool LowerBoundLocal(const TKeyRef& prefix, TKeyRef* firstKey, TData* firstData, const TKeyRef& startKey, const TData& startData) {
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

        TData LastData() const {
            return Buffer_.LastData();
        }

    private:
        void SwapInternal(TTrieReader& other) {
            DoSwap(Table_, other.Table_);
            DoSwap(Input_, other.Input_);
            DoSwap(StreamPosition_, other.StreamPosition_);
            DoSwap(Decoder_, other.Decoder_);
            DoSwap(Buffer_, other.Buffer_);
            Decoder_.Reset(Decoder_.Table(), &Input_);
            other.Decoder_.Reset(other.Decoder_.Table(), &other.Input_);
        }

        bool FillBuffer(const TKeyRef& startKey, const TData& startData) {
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
