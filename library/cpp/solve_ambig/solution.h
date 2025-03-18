#pragma once

#include <util/generic/ptr.h>
#include <util/generic/vector.h>

namespace NSolveAmbig {
    struct TSolution: public TSimpleRefCount<TSolution> {
        TVector<size_t> Positions;
        size_t Coverage;
        double Weight;

        TSolution()
            : Coverage(0)
            , Weight(1.0)
        {
        }
    };

    using TSolutionPtr = TIntrusivePtr<TSolution>;

}
