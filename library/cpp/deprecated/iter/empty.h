#pragma once

#include "base.h"

#include <util/generic/yexception.h>

namespace NIter {
    template <class T>
    class TEmptyIterator: public TBaseIterator<T> {
    public:
        // implements TBaseIterator
        bool Ok() const override {
            return false;
        }

        void operator++() override {
        }

        const T* operator->() const override {
            ythrow yexception() << "Dereferencing empty iterator!";
        }
    };

}
