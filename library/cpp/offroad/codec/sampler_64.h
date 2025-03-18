#pragma once

#include "model_64.h"
#include "standard_sampler.h"

namespace NOffroad {
    class TModel64;

    class TSampler64: public TStandardSampler {
    public:
        enum {
            BlockSize = 64,
            TupleSize = 1,
            Stages = 1,
        };

        using TModel = TModel64;

        TSampler64(size_t maxChunks = 32768)
            : TStandardSampler(64, maxChunks)
        {
        }

        /**
         * Creates an optimized model from this sampler. Note that this function
         * does some heavy lifting under the hood, so it might take some time.
         */
        TModel64 Finish();
    };

}
