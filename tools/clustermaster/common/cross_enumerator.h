#pragma once


#include <util/generic/string.h>
#include <util/generic/vector.h>

class TCrossEnumerator {
private:
    const TVector<TVector<TString> > Dimensions;
    TVector<size_t> Currents;
public:
    TCrossEnumerator(const TVector<TVector<TString> >& dimensions);

    bool Next();

    TVector<TString> operator*() const;

private:
    bool IncAtLevel(size_t level);
};
