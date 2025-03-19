#pragma once

#include <util/generic/vector.h>

template <typename T>
class TSwapVector: public TVector<T> {
public:
    using TParent = TVector<T>;
    using TParent::begin;
    using TParent::end;
    using TParent::pop_back;
    using TParent::push_back;
    using TParent::size;
    using iterator = typename TParent::iterator;
    using const_iterator = typename TParent::const_iterator;

    inline TSwapVector() {
    }

    void insert(const T& newElem, size_t pos) {
        push_back(newElem);
        for (size_t i = size() - 1; i > pos; --i)
            (*this)[i].Swap((*this)[i - 1]);
    }

    void erase(size_t n) {
        Y_ASSERT(n < size());
        for (size_t i = n; i + 1 < size(); ++i) {
            (*this)[i].Swap((*this)[i + 1]);
        }
        pop_back();
    }

    void insert(const T& newElem, iterator i) {
        insert(newElem, i - begin());
    }
    void insert(const T& newElem, const_iterator i) {
        insert(newElem, i - begin());
    }
    void erase(iterator i) {
        erase(i - begin());
    }
    void erase(const_iterator i) {
        erase(i - begin());
    }
};
