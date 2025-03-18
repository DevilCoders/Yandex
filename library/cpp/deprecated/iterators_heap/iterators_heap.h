#pragma once

#include "holders.h"

#include <stdio.h>
#include <stdlib.h>
#include <util/system/defaults.h>

template <typename X, typename THold>
class TDeprecatedIteratorsHeap: public THold {
public:
    typedef typename X::value_type value_type;

protected:
    size_t Count;
    ui64 DummyCounter;
    ui64* SwitchPointer;
    ui64* SkipPointer;

public:
    static value_type MaxValue() {
        return X::MaxValue();
    }

    TDeprecatedIteratorsHeap(X** values, size_t count, ui64* externalSwitchPointer, ui64* externalSkipPointer)
        : Count(count)
        , SwitchPointer(externalSwitchPointer ? externalSwitchPointer : &DummyCounter)
        , SkipPointer(externalSkipPointer ? externalSkipPointer : &DummyCounter)
    {
        Restart(values, count);
    }

    TDeprecatedIteratorsHeap()
        : Count(0)
        , SwitchPointer(&DummyCounter)
        , SkipPointer(&DummyCounter)
    {
    }

    template <class Funct>
    ui64 GetValue(const Funct& f) const {
        ui64 acc = 0;
        for (size_t i = 0; i < Count; ++i) {
            acc += f(this->Pairs[i].Pointer);
        }
        return acc;
    }

    void Normalize() {
        if (this->Pairs[0].Value == this->Pairs[1].Value) {
            X* pointer0 = this->Pairs[0].Pointer;
            X* pointer1 = this->Pairs[1].Pointer;
            if (pointer0 > pointer1) {
                this->Pairs[0].Pointer = pointer1;
                this->Pairs[1].Pointer = pointer0;
            }
        }
    }

    void Y_FORCE_INLINE SiftIter(size_t size, TValuePair<X, value_type>* pv, X* pointer, value_type value) {
        ui64 count = 1;
        while (1) {
            ++count;
            size_t sizeL = 2 * size + 0;
            size_t sizeR = 2 * size + 1;
            value_type valueL = pv[sizeL].Value;
            value_type valueR = pv[sizeR].Value;
            size_t sizeN = (valueL < valueR) ? sizeL : sizeR;
            value_type valueN = (valueL < valueR) ? valueL : valueR;

            if (valueN >= value) {
                pv[size].Value = value;
                pv[size].Pointer = pointer;
                SwitchPointer[0] += count;
                return;
            }
            pv[size].Value = valueN;
            pv[size].Pointer = pv[sizeN].Pointer;
            size = sizeN;
        }
    }
    void Y_FORCE_INLINE SiftTopIter(TValuePair<X, value_type>* pv, X* pointer, value_type value) {
        value_type value1 = pv[1].Value;
        if (value1 >= value) {
            pv[0].Value = value;
            return;
        }
        pv[0].Value = value1;
        pv[0].Pointer = pv[1].Pointer;
        SiftIter(1, pv, pointer, value);
    }

    void Y_FORCE_INLINE SiftTopIter() {
        TValuePair<X, value_type>* pv = &this->Pairs[0];
        X* pointer = pv->Pointer;
        SiftTopIter(pv, pointer, pointer->Current());
    }

    template <class TIt>
    void Restart(TIt values, size_t count) {
        Restart(values, count, [](const TIt it) -> decltype(*it) { return *it; });
    }

    template <class TIt, class Func>
    void Restart(TIt values, size_t count, const Func& f) {
        Count = count;
        this->Resize(count * 2);
        for (size_t i = 0; i < count; ++i, ++values) {
            this->Pairs[i].Pointer = f(values);
        }
        for (size_t i = count; i < count * 2; ++i) {
            this->Pairs[i].Pointer = nullptr;
        }
    }

    void Restart() {
        for (size_t i = 0; i < Count; ++i) {
            this->Pairs[i].Value = this->Pairs[i].Pointer->Current();
        }
        for (size_t i = Count; i < Count * 2; ++i) {
            this->Pairs[i].Value = MaxValue();
        }
        for (size_t i = 0; i < Count; ++i) {
            size_t index = Count - i - 1;
            SiftIter(index, &this->Pairs[0], this->Pairs[index].Pointer, this->Pairs[index].Value);
        }
    }

    X Y_FORCE_INLINE* TopIter() const {
        return this->Pairs[0].Pointer;
    }

    bool Y_FORCE_INLINE Valid() const {
        return this->Pairs[0].Value < X::MaxValue();
    }

    void Y_FORCE_INLINE Next() {
        TValuePair<X, value_type>* pv = &this->Pairs[0];
        X* pointer = pv->Pointer;
        pointer->Next();
        SiftTopIter(pv, pointer, pointer->Current());
    }

    void Touch(size_t size, const value_type& value) {
        while (1) {
            TValuePair<X, value_type>* pv = &this->Pairs[0];
            X* pointer = pv[size].Pointer;
            if (pv[size].Value != value)
                return;
            pointer->Touch();
            value_type cvalue = pointer->Current();
            if (cvalue == value)
                break;
            SiftIter(size, pv, pointer, cvalue);
        }
        Touch(size * 2 + 0, value);
        Touch(size * 2 + 1, value);
    }

    void Y_FORCE_INLINE Touch(const value_type& value) {
        while (1) {
            TValuePair<X, value_type>* pv = &this->Pairs[0];
            X* pointer = pv[0].Pointer;
            if (pv[0].Value != value)
                return;
            pointer->Touch();
            value_type cvalue = pointer->Current();
            if (cvalue == value)
                break;
            SiftTopIter(pv, pointer, cvalue);
        }
        Touch(1, value);
    }

    template <typename TSmartSkip>
    void Y_FORCE_INLINE SkipTo(const value_type& value, const TSmartSkip& skip) {
        ui64 count = 0;
        while (1) {
            TValuePair<X, value_type>* pv = &this->Pairs[0];
            if (pv->Value >= value)
                break;
            X* pointer = pv->Pointer;
            pointer->SkipTo(value, skip);
            ++count;
            SiftTopIter(pv, pointer, pointer->Current());
        }
        SkipPointer[0] += count;
    }

    const value_type Y_FORCE_INLINE Current() const {
        return this->Pairs[0].Value;
    }

    const value_type Y_FORCE_INLINE SecondValue() const {
        return this->Pairs[1].Value;
    }
};
