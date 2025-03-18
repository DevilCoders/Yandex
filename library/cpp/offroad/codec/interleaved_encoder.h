#pragma once

#include <array>
#include <util/generic/array_ref.h>

#include "interleaved_table.h"

namespace NOffroad {
    /**
     * Encoder that writes interleaved data into the underlying stream, using
     * separate encoders for each of the data slices.
     */
    template <size_t tupleSize, class BaseEncoder>
    class TInterleavedEncoder {
    public:
        enum {
            TupleSize = tupleSize * BaseEncoder::TupleSize,
            BlockSize = BaseEncoder::BlockSize,
            Stages = BaseEncoder::Stages
        };

        using TTable = TInterleavedTable<TupleSize, typename BaseEncoder::TTable>;
        using TOutput = typename BaseEncoder::TOutput;

        TInterleavedEncoder() {
        }

        TInterleavedEncoder(const TTable* table, TOutput* output) {
            Reset(table, output);
        }

        void Reset(const TTable* table, TOutput* output) {
            Table_ = table;
            for (size_t i = 0; i < TupleSize; i++)
                Base_[i].Reset(&table->Base(i), output);
        }

        template <class Range>
        void Write(size_t channel, const Range& chunk) {
            Write(channel, MakeArrayRef(chunk));
        }

        template <class Unsigned>
        void Write(size_t channel, const TArrayRef<const Unsigned>& chunk) {
            Y_ASSERT(channel < TupleSize);

            Base_[channel / BaseEncoder::TupleSize].Write(channel % BaseEncoder::TupleSize, chunk);
        }

        const TTable* Table() const {
            return Table_;
        }

    private:
        const TTable* Table_ = nullptr;
        std::array<BaseEncoder, TupleSize> Base_;
    };

}
