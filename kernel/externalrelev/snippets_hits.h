#pragma once

#include <util/generic/fwd.h>

struct TSnippetsHitsContext {
    typedef TVector<SUPERLONG, std::allocator<SUPERLONG> > THitsVector;

    THitsVector& TextHits;
    THitsVector& LinkHits;

    TSnippetsHitsContext(THitsVector& textHits, THitsVector& linkHits)
        : TextHits(textHits)
        , LinkHits(linkHits)
    {
    }
};
