#pragma once

#include "model_16.h"
#include "standard_sampler.h"

namespace NOffroad {
    class TSampler16: public TStandardSampler {
    public:
        enum {
            BlockSize = 16,
            TupleSize = 1,
            Stages = 1,
        };

        using TModel = TModel16;

        TSampler16(size_t maxChunks = 32768)
            : TStandardSampler(16, maxChunks)
        {
        }

        /**
         * Creates an optimized model from this sampler. Note that this function
         * does some heavy lifting under the hood, so it might take some time.
         */
        TModel16 Finish();
    };

}
