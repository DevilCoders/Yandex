#pragma once

#include "slave_copy_points.h"

/*
 * Some template typedefs
 */
template <class T>
class TSlaveInputPoint: public TSlaveCopyPoint<void (*)(void*, T)> {
public:
    typedef void (*TCopyFunction)(void*, T);

private:
    typedef TSlaveCopyPoint<TCopyFunction> TBase;

    friend class TSimpleModule;

    template <class TOwnerType>
    class TBinderData {
    public:
        typedef TOwnerType TOwner;
        typedef void (TOwner::*TMethod)(T);
        typedef TCopyFunction TValue;

        template <TMethod f>
        static void Call(void* owner, T t) {
            (reinterpret_cast<TOwner*>(owner)->*f)(t);
        }
    };

public:
    TSlaveInputPoint(void* owner, TCopyFunction val)
        : TBase(owner, val)
    {
    }

    template <class TConcreteBinderData>
    TSlaveInputPoint(NPointValueBinders::TOwnerValueBinder<TConcreteBinderData> param)
        : TBase(param)
    {
    }

    template <class TOwner>
    static NPointValueBinders::TOwnerBinder<TBinderData<TOwner>> Bind(TOwner* owner) {
        return NPointValueBinders::TOwnerBinder<TBinderData<TOwner>>(owner);
    }
};

template <class T>
class TSlaveOutputPoint: public TSlaveCopyPoint<T (*)(void*)> {
public:
    typedef T (*TCopyFunction)(void*);

private:
    typedef TSlaveCopyPoint<TCopyFunction> TBase;

    friend class TSimpleModule;

    template <class TOwnerType>
    class TBinderData {
    public:
        typedef TOwnerType TOwner;
        typedef T (TOwner::*TMethod)();
        typedef TCopyFunction TValue;

        template <TMethod f>
        static T Call(void* owner) {
            return (reinterpret_cast<TOwner*>(owner)->*f)();
        }
    };

public:
    TSlaveOutputPoint(void* owner, TCopyFunction val)
        : TBase(owner, val)
    {
    }

    template <class TConcreteBinderData>
    TSlaveOutputPoint(NPointValueBinders::TOwnerValueBinder<TConcreteBinderData> param)
        : TBase(param)
    {
    }

    template <class TOwner>
    static NPointValueBinders::TOwnerBinder<TBinderData<TOwner>> Bind(TOwner* owner) {
        return NPointValueBinders::TOwnerBinder<TBinderData<TOwner>>(owner);
    }
};
