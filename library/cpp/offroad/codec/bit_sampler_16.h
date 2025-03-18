#pragma once

#include "model_64.h"
#include "standard_sampler.h"

namespace NOffroad {
    class TModel64;

    /**
     * Sampler for `TBitEncoder16`.
     */
    class TBitSampler16: public TStandardSampler {
    public:
        enum {
            TupleSize = 1,
            BlockSize = 16,
            Stages = 1,
        };

        using TModel = TModel64;

        TBitSampler16(size_t maxChunks = 32768)
            : TStandardSampler(16, maxChunks)
        {
        }

        /**
         * Creates an optimized model from this sampler. Note that this function
         * does some heavy lifting under the hood, so it might take some time.
         */
        TModel64 Finish();
    };

}
