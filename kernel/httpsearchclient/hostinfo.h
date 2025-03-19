#pragma once

#include <kernel/index_generation/constants.h>

namespace NHttpSearchClient {

    class THostInfo {
    public:
        THostInfo()
            : Nctx(0)
            , Ncpu(0)
            , IndexGeneration(UndefIndGenValue)
            , SourceTimestamp(0)
            , BsTouched(0)
            , UnanswerCount(0)
            , Accessibility(-1.0)
            , IsSearch(true)
        {
        }

        size_t Nctx;
        size_t Ncpu;
        ui32 IndexGeneration;
        ui32 SourceTimestamp;
        ui32 BsTouched;
        size_t UnanswerCount;
        double Accessibility;
        bool IsSearch;
    };

}
