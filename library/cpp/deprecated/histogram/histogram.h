#pragma once

#include <algorithm>
#include <numeric>

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/generic/utility.h>
#include <util/generic/ymath.h>
#include <util/generic/yexception.h>

#include <util/random/mersenne.h>

#include <util/stream/output.h>

#include <util/system/yassert.h>

template <class T>
class TIntHistogram: public TVector<T> {
public:
    T& operator[](size_t i) {
        if (i >= TVector<T>::size())
            TVector<T>::resize(i + 1, T(0));
        return TVector<T>::operator[](i);
    }

    T operator[](size_t i) const {
        return TVector<T>::operator[](i);
    }

    void AddTo(size_t i) {
        ++(operator[](i));
    }

    void operator+=(const TIntHistogram<T>& rhs) {
        const TVector<T>& rhs1 = rhs;
        size_t addSize = Min(TVector<T>::size(), rhs1.size());
        for (size_t i = 0; i < addSize; ++i)
            (*this)[i] += rhs1[i];

        if (rhs1.size() > TVector<T>::size())
            TVector<T>::insert(TVector<T>::end(), rhs1.begin() + addSize, rhs1.end());
    }

    void Trim(size_t nMax) {
        if (TVector<T>::size() > nMax)
            TVector<T>::resize(nMax);
    }

    void Print(IOutputStream& out,
               bool vertical = true, bool printArgs = true, bool printEmpty = true) const {
        size_t nParts = TVector<T>::size();
        if (nParts == 0)
            return;

        if (vertical) {
            for (size_t i = 0; i < nParts; ++i) {
                T v = (operator[](i));
                if ((v == 0) && !printEmpty)
                    continue;
                if (printArgs)
                    out << i << '\t';
                out << v << '\n';
            }
        } else {
            if (printArgs) {
                bool first = true;
                for (size_t i = 0; i < nParts; ++i) {
                    T v = (operator[](i));
                    if ((v == 0) && !printEmpty)
                        continue;
                    if (first) {
                        first = false;
                    } else {
                        out << '\t';
                    }
                    out << i;
                }
                out << '\n';
            }
            bool first = true;
            for (size_t i = 0; i < nParts; ++i) {
                T v = (operator[](i));
                if ((v == 0) && !printEmpty)
                    continue;
                if (first) {
                    first = false;
                } else {
                    out << '\t';
                }
                out << v;
            }
            out << '\n';
        }
    }
};
