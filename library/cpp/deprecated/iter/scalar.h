#pragma once

#include "base.h"

namespace NIter {
    template <class T>
    class TScalarIterator: public TBaseIterator<T> {
    public:
        // @value must exist during whole TScalarIterator's life-time
        inline explicit TScalarIterator(const T* value)
            : Value(value)
        {
        }

        bool Ok() const override {
            return Value != nullptr;
        }

        void operator++() override {
            Value = nullptr;
        }

        const T* operator->() const override {
            return Value;
        }

    private:
        const T* Value;
    };

}
