#pragma once

#include <library/cpp/deprecated/atomic/atomic.h>

template<class T>
class TObjectIdentifier {
public:
    typedef std::make_unsigned<TAtomicBase>::type TId;
private:
    static TAtomic Counter;
    TId Id;
public:
    TObjectIdentifier()
     : Id(static_cast<TId>(AtomicIncrement(Counter)))
    {
    }

    TAtomicBase GetId() const {
        return Id;
    }
};

template<class T>
TAtomic TObjectIdentifier<T>::Counter = typename TObjectIdentifier<T>::TId(0);
