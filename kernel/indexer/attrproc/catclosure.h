#pragma once

#include <util/generic/hash_set.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>

typedef THashSet<ui32> TCatSet;

class TCatClosure {
private:
    typedef THashMap<ui64, TVector<ui32> > TCatTransit;
private:
    ui32        Zero;
    TCatTransit Map;
public:
    TCatClosure(const char* fname);
    const ui32* Get(ui32 cat) const;
    void DoClosure(const TCatSet& in, TCatSet* out) const;
};
