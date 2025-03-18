#pragma once

#include <util/generic/utility.h>

#include <library/cpp/offroad/codec/encoder_64.h>
#include <library/cpp/offroad/codec/interleaved_encoder.h>
#include <library/cpp/offroad/streams/vec_output.h>
#include <library/cpp/offroad/offset/data_offset.h>

#include "tuple_output_buffer.h"

namespace NOffroad {
    /**
     * Writer for offroad-encoded tuple streams.
     *
     * Provides a convenient interface that abstracts away all the underlying magic
     * with regards to how the data is split into integers, delta-encoded, and how
     * the underlying offroad streams are configured.
     *
     * \tparam Data                         Type of the tuples to write.
     * \tparam Vectorizer                   Vectorizer that is going to be used to split
     *                                      input data into tuples of integers.
     * \tparam Subtractor                   Delta-encoder specific to the data
     *                                      and the vectorizer provided.
     * \tparam BaseEncoder                  Underlying offroad encoder to use. This one
     *                                      also determines block size.
     * \tparam blockMultiplier              Number of underlying blocks (in BaseEncoder terms) in one
     *                                      block exposed by this writer.
     * \tparam bufferType                   Buffer type that will be used. Determines how EOFs
     *                                      are handled by this writer.
     *
     * \see TTupleSampler
     */
    template <class Data, class Vectorizer, class Subtractor, class BaseEncoder = TEncoder64, size_t blockMultiplier = 1, EBufferType bufferType = AutoEofBuffer>
    class TTupleWriter {
        using TEncoder = TInterleavedEncoder<Vectorizer::TupleSize, BaseEncoder>;
        using TOutput = typename TEncoder::TOutput;

    public:
        using THit = Data;
        using TTable = typename TEncoder::TTable;
        using TModel = typename TTable::TModel;
        using TPosition = TDataOffset;

        enum {
            TupleSize = Vectorizer::TupleSize,
            BlockSize = TEncoder::BlockSize * blockMultiplier,
            Stages = TEncoder::Stages
        };

        TTupleWriter() {
        }

        TTupleWriter(const TTable* table, IOutputStream* output) {
            Reset(table, output);
        }

        TTupleWriter(const TTupleWriter&) = delete;

        TTupleWriter(TTupleWriter&& other) {
            SwapInternal(other);
        }

        ~TTupleWriter() {
            Finish();
        }

        void Reset(const TTable* table, IOutputStream* output) {
            Output_.Reset(output);
            Encoder_.Reset(table, &Output_);
            Buffer_.Reset();
        }

        /**
         * Writes provided data into this stream.
         *
         * @param data                      Data to write.
         */
        void WriteHit(const THit& data) {
            Buffer_.WriteHit(data);

            if (Buffer_.IsDone())
                FlushBuffer();
        }

        void WriteSeekPoint() {
            Buffer_.WriteSeekPoint();
        }

        /**
         * Flushes all the data from this stream's buffers, pads it with zeros and
         * closes this stream. No more writing is possible once the stream is closed.
         */
        void Finish() {
            if (IsFinished())
                return;

            Buffer_.Finish();
            FlushBuffer();
            Output_.Finish();
        }

        bool IsFinished() const {
            return Output_.IsFinished();
        }

        /**
         * @returns                         Current output position.
         */
        TDataOffset Position() const {
            Y_ASSERT(!Buffer_.IsDone());

            return TDataOffset(Output_.Position(), Buffer_.BlockPosition());
        }

    private:
        void SwapInternal(TTupleWriter& other) {
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
        TOutput Output_;
        TEncoder Encoder_;
        TTupleOutputBuffer<THit, Vectorizer, Subtractor, BlockSize, bufferType> Buffer_;
    };

}
