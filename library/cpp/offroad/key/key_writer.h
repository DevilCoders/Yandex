#pragma once

#include <library/cpp/offroad/codec/interleaved_encoder.h>
#include <library/cpp/offroad/codec/encoder_64.h>
#include <library/cpp/offroad/offset/data_offset.h>

#include "key_output_buffer.h"
#include "key_table.h"

namespace NOffroad {
    template <class KeyData, class Vectorizer, class Subtractor, EKeySubtractor keySubtractor = DeltaKeySubtractor>
    class TKeyWriter {
        using TOutputBuffer = TKeyOutputBuffer<KeyData, Vectorizer, Subtractor, 64, keySubtractor>;
        using TEncoder = TInterleavedEncoder<TOutputBuffer::TupleSize, TEncoder64>;

    public:
        using TKey = TString;
        using TKeyRef = TStringBuf;
        using TKeyData = KeyData;
        using TTable = TKeyTable<typename TEncoder::TTable>;
        using TModel = typename TTable::TModel;
        using TPosition = TDataOffset;

        enum {
            BlockSize = 64,
            Stages = TEncoder::Stages
        };

        TKeyWriter() {
        }

        TKeyWriter(const TTable* table, IOutputStream* output) {
            Reset(table, output);
        }

        TKeyWriter(const TKeyWriter&) = delete;

        TKeyWriter(TKeyWriter&& other) {
            SwapInternal(other);
        }

        ~TKeyWriter() {
            Finish();
        }

        void Reset(const TTable* table, IOutputStream* output) {
            Table_ = table;
            Output_.Reset(output);
            Encoder_.Reset(&table->Base(), &Output_);
            Buffer_.Reset();
        }

        void WriteKey(const TKeyRef& key, const TKeyData& data) {
            Buffer_.Write(key, data);

            if (Buffer_.IsDone())
                FlushBuffer();
        }

        void Finish() {
            if (IsFinished())
                return;

            if (Buffer_.BlockPosition() == 0) {
                Output_.Finish();
            } else {
                Buffer_.Finish();
                FlushBuffer();
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
        void SwapInternal(TKeyWriter& other) {
            DoSwap(Table_, other.Table_);
            DoSwap(Output_, other.Output_);
            DoSwap(Encoder_, other.Encoder_);
            DoSwap(Buffer_, other.Buffer_);
            Encoder_.Reset(Encoder_.Table(), &Output_);
            other.Encoder_.Reset(other.Encoder_.Table(), &other.Output_);
        }

        void FlushBuffer() {
            Buffer_.Flush(&Encoder_);
        }

    private:
        const TTable* Table_;
        TVecOutput Output_;
        TEncoder Encoder_;
        TOutputBuffer Buffer_;
    };

}
