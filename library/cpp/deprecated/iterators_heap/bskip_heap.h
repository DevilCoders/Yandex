#pragma once

#include "holders.h"

template <typename X, typename Value, typename THold>
class TDeprecatedBSkipHeap: public THold {
public:
    typedef Value value_type;

protected:
    size_t Count;

public:
    size_t Size() const {
        return Count;
    }

    TDeprecatedBSkipHeap()
        : Count(0)
    {
    }

    TDeprecatedBSkipHeap(X** values, size_t count)
        : Count(count)
    {
        Restart(values, count);
    }

    void Bubble(TValuePair<X, value_type>* pairs, value_type value) {
        X* pointer = pairs[0].Pointer;
        TValuePair<X, value_type>* pv = pairs;

        for (; pv[1].Value < value; ++pv) {
            pv[0].Value = pv[1].Value;
            pv[0].Pointer = pv[1].Pointer;
        }
        pv[0].Pointer = pointer;
        pv[0].Value = value;
    }

    void Restart(X** values, size_t count) {
        Count = count;
        this->Resize(count);
        for (size_t i = 0; i < count; ++i)
            this->Pairs[i].Pointer = values[i];
    }

    void Restart() {
        for (size_t i = 0; i < Count; ++i) {
            this->Pairs[i].Value = X::MinValue();
        }
        this->Pairs[Count].Value = X::MaxValue();
    }

    X* TopIter() const {
        return this->Pairs[0].Pointer;
    }

    const value_type TopValue() const {
        return this->Pairs[0].Value;
    }

    value_type MaxValue() const {
        return X::MaxValue();
    }

    bool Valid() const {
        return this->Pairs[0].Value < X::MaxValue();
    }

    void SiftTopIter(value_type value) {
        Bubble(&this->Pairs[0], value);
    }

    const value_type SecondValue() const {
        return this->Pairs[1].Value;
    }
};
