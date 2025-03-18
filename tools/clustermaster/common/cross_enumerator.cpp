#include "cross_enumerator.h"

#include <util/system/yassert.h>

TCrossEnumerator::TCrossEnumerator(const TVector<TVector<TString> >& dimensions)
    : Dimensions(dimensions)
{
    Y_VERIFY(Dimensions.size() > 0, "must have at least one dim");
}


bool TCrossEnumerator::Next() {
    if (Currents.empty()) {
        for (TVector<TVector<TString> >::const_iterator dim = Dimensions.begin();
                dim != Dimensions.end(); ++dim)
        {
            if (dim->empty())
                return false;
        }

        Currents.resize(Dimensions.size(), 0);
        return true;
    }

    return IncAtLevel(Dimensions.size() - 1);
}

bool TCrossEnumerator::IncAtLevel(size_t level) {
    Y_VERIFY(level < Dimensions.size(), "too large");

    if (Currents.at(level) + 1 == Dimensions.at(level).size()) {
        if (level == 0)
            return false;
        return IncAtLevel(level - 1);
    }

    Currents.at(level) += 1;
    for (size_t i = level + 1; i < Dimensions.size(); ++i) {
        Currents.at(i) = 0;
    }

    return true;
}

TVector<TString> TCrossEnumerator::operator*() const {
    Y_VERIFY(Currents.size() == Dimensions.size(), "bla bla");
    TVector<TString> r;
    for (size_t i = 0; i < Dimensions.size(); ++i) {
        r.push_back(Dimensions.at(i).at(Currents.at(i)));
    }
    return r;
}
