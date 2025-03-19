#pragma once

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
#error This header file is intended for use in the MATRIXNET_WITHOUT_ARCADIA mode only.
#endif

#include <vector>

namespace NMatrixnet {

template<typename T>
class TVector : public std::vector<T> {
public:
    using TBase = std::vector<T>;
    TVector() {}

    TVector(const TVector& src)
        : TBase(src) {}

    TVector& operator =(const TVector& src) {
        TBase::operator =(src);
        return *this;
    }

    template <class TIter>
    TVector(TIter first, TIter last)
        : TBase(first, last) {}

    explicit TVector(typename TBase::size_type count)
        : TBase(count) {}

    explicit TVector(typename TBase::size_type count, const T& val)
        : TBase(count, val) {}

    typename TBase::size_type operator+() const {
        return TBase::size();
    }

    T* operator~() {
        return TBase::data();
    }

    const T* operator~() const {
        return TBase::data();
    }

    int ysize() const {
        return static_cast<int>(TBase::size());
    }

    void Swap(TVector<T>& other) {
        TBase::swap(other);
    }
};

} // namespace NMatrixnet
