#pragma once

#include <util/generic/hash_set.h>

template <class T>
class TIncExcFilter {
    THashSet<T> Inc;
    THashSet<T> Exc;

public:
    TIncExcFilter() {
    }

    TIncExcFilter(const THashSet<T>& inc,
                  const THashSet<T>& exc = THashSet<T>())
        : Inc(inc)
        , Exc(exc)
    {
    }

    void AddInc(const T& e) {
        Inc.insert(e);
    }
    void AddExc(const T& e) {
        Exc.insert(e);
    }

    bool Check(const T& e) const {
        if (!Inc.empty() && !Inc.contains(e)) {
            return false;
        }
        if (Exc.contains(e)) {
            return false;
        }
        return true;
    }
};
