#pragma once

#include <type_traits>

#include <util/system/yassert.h>
#include <util/generic/array_ref.h>

#include <library/cpp/offroad/streams/vec_output.h>

#include "private/encoder_table.h"

#include "table_16.h"

namespace NOffroad {
    /**
     * Encoder that writes into a vector stream and operates on chunks of 16 integers.
     *
     * Example usage:
     * @code
     * TVector<std::array<ui32, 16>> data = YourData();
     *
     * TSampler sampler(16);
     * for(const auto& chunk: data)
     *     sampler.Write(chunk);
     * TModel16 model = sampler.Finish();
     *
     * IOutputStream* stream = YourStream();
     * TVecOutput output(stream);
     * TEncoder16 compressor(model, &output);
     *
     * for(const auto& chunk: data)
     *     compressor.Write(chunk);
     * output.Finish();
     *
     * // Now you have compressed data in `stream`,
     * // and a model to decompress it in `model`.
     * @endcode
     */
    class TEncoder16 {
    public:
        enum {
            TupleSize = 1,
            BlockSize = 16,
            Stages = 1
        };

        using TTable = TEncoder16Table;
        using TOutput = TVecOutput;

        TEncoder16() = default;

        TEncoder16(const TTable* table, TVecOutput* output) {
            Reset(table, output);
        }

        void Reset(const TTable* table, TVecOutput* output) {
            Table_ = table;
            Output_ = output;
        }

        template <class Range>
        void Write(size_t channel, const Range& chunk) {
            Write(channel, MakeArrayRef(chunk));
        }

        /**
         * Compresses and writes out provided data block of size 16 into the
         * underlying stream.
         */
        template <class Unsigned>
        void Write(size_t channel, const TArrayRef<const Unsigned>& block) {
            NPrivate::AssertSupportedType<Unsigned>();

            Y_ASSERT(block.size() == BlockSize);
            Y_ASSERT(channel == 0);
            Y_UNUSED(channel);

            ui32 levels[4];
            ui32 schemes[4];

            for (size_t i = 0; i < 4; ++i)
                Table_->Base().CalculateParams(NPrivate::LoadVec(&block[i * 4]), &levels[i], &schemes[i]);

            Output_->Write(TVec4u(&schemes[0]), 8);
            for (size_t i = 0; i < 4; ++i)
                Output_->Write(NPrivate::LoadVec(&block[i * 4]) & VectorMask(levels[i]), levels[i]);
        }

        const TTable* Table() const {
            return Table_;
        }

    private:
        const TTable* Table_ = nullptr;
        TVecOutput* Output_ = nullptr;
    };

}
