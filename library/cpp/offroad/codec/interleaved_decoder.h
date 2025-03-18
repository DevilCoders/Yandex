#pragma once

#include <array>

#include <util/generic/array_ref.h>

#include "interleaved_table.h"

namespace NOffroad {
    /**
     * Decoder that reads from an interleaved stream, using separate base decoder
     * for each slice of data.
     */
    template <size_t tupleSize, class BaseDecoder>
    class TInterleavedDecoder {
    public:
        enum {
            TupleSize = tupleSize * BaseDecoder::TupleSize,
            BlockSize = BaseDecoder::BlockSize
        };

        using TTable = TInterleavedTable<TupleSize, typename BaseDecoder::TTable>;
        using TInput = typename BaseDecoder::TInput;

        TInterleavedDecoder() = default;

        TInterleavedDecoder(const TTable* table, TInput* input) {
            Reset(table, input);
        }

        void Reset(const TTable* table, TInput* input) {
            Table_ = table;
            for (size_t i = 0; i < TupleSize; i++)
                Base_[i].Reset(table ? &table->Base(i) : nullptr, input);
        }

        template <class Range>
        size_t Read(size_t channel, Range* chunk) {
            return Read(channel, MakeArrayRef(*chunk));
        }

        template <class Unsigned>
        size_t Read(size_t channel, TArrayRef<Unsigned> chunk) {
            Y_ASSERT(channel < TupleSize);

            return Base_[channel / BaseDecoder::TupleSize].Read(channel % BaseDecoder::TupleSize, chunk);
        }

        const TTable* Table() const {
            return Table_;
        }

    private:
        const TTable* Table_ = nullptr;
        std::array<BaseDecoder, TupleSize> Base_;
    };

}
