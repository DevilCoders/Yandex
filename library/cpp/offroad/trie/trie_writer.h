#pragma once

#include <util/system/yassert.h>
#include <util/generic/ptr.h>

#include <library/cpp/offroad/codec/interleaved_encoder.h>
#include <library/cpp/offroad/codec/encoder_64.h>
#include <library/cpp/offroad/offset/data_offset.h>

#include "trie_output_buffer.h"

namespace NOffroad {
    class TTrieWriter {
        using TOutputBuffer = TTrieOutputBuffer<64>;
        using TEncoder = TInterleavedEncoder<TOutputBuffer::TupleSize, TEncoder64>;

    public:
        using TKey = TString;
        using TKeyRef = TStringBuf;
        using TData = TStringBuf;
        using TTable = typename TEncoder::TTable;
        using TModel = typename TTable::TModel;

        enum {
            BlockSize = 64,
        };

        TTrieWriter() {
        }

        TTrieWriter(const TTable* table, IOutputStream* output) {
            Reset(table, output);
        }

        TTrieWriter(const TTrieWriter&) = delete;

        TTrieWriter(TTrieWriter&& other) {
            SwapInternal(other);
        }

        ~TTrieWriter() {
            Finish();
        }

        void Reset(const TTable* table, IOutputStream* output) {
            Table_ = table;
            Output_.Reset(output);
            Encoder_.Reset(table, &Output_);
            Buffer_.Reset();
        }

        void Write(const TKeyRef& key, const TData& data) {
            Buffer_.Write(key, data);

            if (Buffer_.IsDone())
                Buffer_.Flush(&Encoder_);
        }

        void Finish() {
            if (IsFinished())
                return;

            if (Buffer_.BlockPosition() == 0) {
                Output_.Finish();
            } else {
                Buffer_.Finish();
                Buffer_.Flush(&Encoder_);
                Output_.Finish();
            }
        }

        bool IsFinished() const {
            return Output_.IsFinished();
        }

        TDataOffset Position() const {
            return TDataOffset(Output_.Position(), Buffer_.BlockPosition());
        }

        const TTable* Table() const {
            return Table_;
        }

        const TKey& LastKey() const {
            return Buffer_.LastKey();
        }

    private:
        void SwapInternal(TTrieWriter& other) {
            DoSwap(Table_, other.Table_);
            DoSwap(Output_, other.Output_);
            DoSwap(Encoder_, other.Encoder_);
            DoSwap(Buffer_, other.Buffer_);
            Encoder_.Reset(Encoder_.Table(), &Output_);
            other.Encoder_.Reset(other.Encoder_.Table(), &other.Output_);
        }

    private:
        const TTable* Table_;
        TVecOutput Output_;
        TEncoder Encoder_;
        TOutputBuffer Buffer_;
    };

}
