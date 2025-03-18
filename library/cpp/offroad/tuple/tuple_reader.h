#pragma once

#include <util/generic/array_ref.h>
#include <util/generic/utility.h>

#include <library/cpp/offroad/codec/decoder_64.h>
#include <library/cpp/offroad/codec/interleaved_decoder.h>
#include <library/cpp/offroad/streams/vec_input.h>
#include <library/cpp/offroad/offset/data_offset.h>

#include "tuple_input_buffer.h"

namespace NOffroad {
    /**
     * Reader for offroad-encoded tuple streams.
     *
     * Provides a convenient interface that abstracts away all the underlying magic
     * with regards to how the data is split into integers, delta-encoded, and how
     * the underlying offroad streams are configured.
     *
     * \tparam Data                         Type of the tuples to read.
     * \tparam Vectorizer                   Vectorizer that is going to be used to construct
     *                                      data objects from tuples of integers.
     * \tparam Subtractor                   Delta-decoder specific to the data
     *                                      and the vectorizer provided.
     * \tparam BaseDecoder                  Underlying offroad decoder to use. This one
     *                                      also determines block size.
     * \tparam blockMultiplier              Number of underlying blocks (in BaseDecoder terms) in one
     *                                      block exposed by this reader.
     * \tparam bufferType                   Buffer type that will be used. Determines how EOFs
     *                                      are handled by this reader.
     */
    template <class Data, class Vectorizer, class Subtractor, class BaseDecoder = TDecoder64, size_t blockMultiplier = 1, EBufferType bufferType = AutoEofBuffer>
    class TTupleReader {
        using TDecoder = TInterleavedDecoder<Vectorizer::TupleSize, BaseDecoder>;
        using TInput = typename TDecoder::TInput;

    public:
        using THit = Data;
        using TTable = typename TDecoder::TTable;
        using TModel = typename TTable::TModel;
        using TPosition = TDataOffset;

        enum {
            TupleSize = Vectorizer::TupleSize,
            BlockSize = TDecoder::BlockSize * blockMultiplier,
        };

        TTupleReader() {
            Decoder_.Reset(nullptr, &Input_);
        }

        TTupleReader(const TTable* table, const TArrayRef<const char>& source) {
            Reset(table, source);
        }

        TTupleReader(const TTupleReader&) = delete;

        TTupleReader(TTupleReader&& other) {
            SwapInternal(other);
        }

        void Swap(TTupleReader& other) {
            SwapInternal(other);
        }

        void Reset() {
            Reset(nullptr, TArrayRef<const char>());
        }

        void Reset(const TTable* table, const TArrayRef<const char>& source) {
            Input_.Reset(source);
            ResetInternal(table);
        }

        void Reset(const TTable* table, const TBlob& blob) {
            Input_.Reset(blob);
            ResetInternal(table);
        }

        void Restart() {
            Input_.Seek(0);
            StreamPosition_ = 0;
            Buffer_.Reset();
        }

        /**
         * Reads the next data from this stream.
         *
         * @param[out] data                 Data to read.
         * @returns                         Whether the data was read. A return value
         *                                  of false signals end of stream.
         */
        Y_FORCE_INLINE bool ReadHit(THit* data) {
            if (Buffer_.IsDone())
                if (!FillBuffer(Buffer_.Last()))
                    return false;

            Buffer_.Read(data);
            return true;
        }

        /**
         * @param consumer                  Hit consumer functor. Takes a hit, returns whether the hit was accepted.
         * @returns                         True iff can read more data.
         */
        template <class Consumer>
        Y_FORCE_INLINE bool ReadHits(const Consumer& consumer) {
            if (!Buffer_.ReadHits(consumer)) {
                return false;
            }
            return FillBuffer(Buffer_.Last());
        }

        /**
         * Seeks into provided position in this stream. If a seek fails, further
         * reading will produce indeterminate results.
         *
         * @param position                  Position to seek into.
         * @param startData                 Initial data value to use when integrating
         *                                  the chunk at given position.
         * @param mode                      Seek mode to use, either `TIntegratingSeek`
         *                                  or `TSeekPointSeek`.
         * @returns                         Whether the seek operation was successful.
         *                                  A return value of false signals end of stream.
         */
        template <class SeekMode = TIntegratingSeek>
        bool Seek(TDataOffset position, const THit& startData, SeekMode mode = SeekMode()) {
            return SeekInternal(position, startData, mode);
        }

        /**
         * Performs a lower bound seek in current block. If provided prefix is not
         * in the current block, returns false.
         */
        bool LowerBoundLocal(const THit& prefix, THit* first) {
            return Buffer_.LowerBound(prefix, first, 0, BlockSize, StreamPosition_ == 0);
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

        THit Last() const {
            return Buffer_.Last();
        }

    protected:
        template <class SeekMode>
        bool SeekInternal(TDataOffset position, const THit& startData, SeekMode mode, size_t integrationShift = 0, bool reloadBlock = false) {
            ui64 streamPosition = position.Offset();
            size_t blockPosition = position.Index();

            if (streamPosition != StreamPosition_ || Y_UNLIKELY(Buffer_.IsEmpty()) || reloadBlock) {
                if (!Input_.Seek(streamPosition))
                    return false;

                if (!FillBuffer(startData, integrationShift))
                    return false;
            }

            return Buffer_.Seek(blockPosition, startData, mode);
        }

        void SwapInternal(TTupleReader& other) {
            DoSwap(Input_, other.Input_);
            DoSwap(StreamPosition_, other.StreamPosition_);
            DoSwap(Decoder_, other.Decoder_);
            DoSwap(Buffer_, other.Buffer_);
            Decoder_.Reset(Decoder_.Table(), &Input_);
            other.Decoder_.Reset(other.Decoder_.Table(), &other.Input_);
        }

        void ResetInternal(const TTable* table) {
            Decoder_.Reset(table, &Input_);
            StreamPosition_ = 0;
            Buffer_.Reset();
        }

        bool FillBuffer(const THit& startData, size_t integrationShift = 0) {
            StreamPosition_ = Input_.Position();
            return Buffer_.Fill(&Decoder_, startData, integrationShift);
        }

    protected:
        TInput Input_;
        ui64 StreamPosition_ = 0;
        TDecoder Decoder_;
        TTupleInputBuffer<THit, Vectorizer, Subtractor, BlockSize, bufferType> Buffer_;
    };

}
