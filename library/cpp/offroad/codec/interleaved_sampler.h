#pragma once

#include <array>
#include <util/generic/array_ref.h>

#include "interleaved_model.h"

namespace NOffroad {
    template <size_t tupleSize, class BaseSampler>
    class TInterleavedSampler {
    public:
        enum {
            TupleSize = tupleSize * BaseSampler::TupleSize,
            BlockSize = BaseSampler::BlockSize,
            Stages = BaseSampler::Stages,
        };

        using TModel = TInterleavedModel<TupleSize, typename BaseSampler::TModel>;

        TInterleavedSampler(size_t maxChunks = 32768) {
            SetMaxChunks(maxChunks);
        }

        void Reset() {
            for (size_t i = 0; i < TupleSize; i++)
                Base_[i].Reset();
        }

        size_t MaxChunks() const {
            return Base_[0].MaxChunks();
        }

        void SetMaxChunks(size_t maxChunks) {
            for (size_t i = 0; i < TupleSize; i++)
                Base_[i].SetMaxChunks(maxChunks);
        }

        template <class Range>
        void Write(size_t channel, const Range& chunk) {
            Write(channel, MakeArrayRef(chunk));
        }

        template <class Unsigned>
        void Write(size_t channel, const TArrayRef<const Unsigned>& chunk) {
            Y_ASSERT(channel < TupleSize);

            Base_[channel / BaseSampler::TupleSize].Write(channel % BaseSampler::TupleSize, chunk);
        }

        TModel Finish() {
            TModel result;
            if (IsFinished())
                return result;

            for (size_t i = 0; i < TupleSize; i++)
                result.Base_[i] = Base_[i].Finish();

            return result;
        }

        bool IsFinished() const {
            return Base_[0].IsFinished();
        }

    private:
        std::array<BaseSampler, TupleSize> Base_;
    };

}
