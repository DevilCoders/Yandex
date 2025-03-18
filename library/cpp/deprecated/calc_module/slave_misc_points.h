#pragma once

#include "slave_copy_points.h"

template <class... A>
class TSlaveNArgsPoint: public TSlaveCopyPoint<void (*)(void*, A...)> {
public:
    typedef void (*TNArgsFunction)(void*, A...);

private:
    typedef TSlaveCopyPoint<TNArgsFunction> TBase;

    friend class TSimpleModule;

    template <class TOwnerType>
    class TBinderData {
    public:
        typedef TOwnerType TOwner;
        typedef void (TOwner::*TMethod)(A...);
        typedef TNArgsFunction TValue;

        template <TMethod f>
        static void Call(void* owner, A... a) {
            (reinterpret_cast<TOwner*>(owner)->*f)(a...);
        }
    };

public:
    TSlaveNArgsPoint(void* owner, TNArgsFunction val)
        : TBase(owner, val)
    {
    }

    template <class TConcreteBinderData>
    TSlaveNArgsPoint(NPointValueBinders::TOwnerValueBinder<TConcreteBinderData> param)
        : TBase(param)
    {
    }

    template <class TOwner>
    static NPointValueBinders::TOwnerBinder<TBinderData<TOwner>> Bind(TOwner* owner) {
        return NPointValueBinders::TOwnerBinder<TBinderData<TOwner>>(owner);
    }
};

template <class A>
using TSlaveArgsPoint = TSlaveNArgsPoint<A>;
template <class A, class B>
using TSlave2ArgsPoint = TSlaveNArgsPoint<A, B>;
template <class A, class B, class C>
using TSlave3ArgsPoint = TSlaveNArgsPoint<A, B, C>;
