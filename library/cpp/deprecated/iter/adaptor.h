#pragma once

#include "base.h"

namespace NIter {
    template <class TIter, class T = typename TIter::TValue>
    class TIteratorAdaptor: public TBaseIterator<T> {
    public:
        // transform any generic iterator with TBaseIterator-like interface to TBaseIterator
        inline TIteratorAdaptor(const TIter& iter)
            : Iter(iter)
        {
        }

        bool Ok() const override {
            return Iter.Ok();
        }

        void operator++() override {
            ++Iter;
        }

        const T* operator->() const override {
            return Iter.operator->();
        }

    private:
        TIter Iter;
    };

}
