#pragma once

#include "slave_copy_points.h"

#include <library/cpp/deprecated/atomic/atomic.h>

/*
 * Some template typedefs
 */
typedef void (*TActionFunction)(void*);
class TSlaveActionPoint: public TSlaveCopyPoint<TActionFunction> {
private:
    typedef TSlaveCopyPoint<TActionFunction> TBase;

    friend class TSimpleModule;

    template <class TOwnerType>
    class TBinderData {
    public:
        typedef TOwnerType TOwner;
        typedef void (TOwner::*TMethod)();
        typedef TActionFunction TValue;

        template <TMethod f>
        static void Call(void* owner) {
            (reinterpret_cast<TOwner*>(owner)->*f)();
        }
    };

public:
    TSlaveActionPoint(void* owner, TActionFunction val)
        : TBase(owner, val)
    {
    }

    template <class TConcreteBinderData>
    TSlaveActionPoint(NPointValueBinders::TOwnerValueBinder<TConcreteBinderData> param)
        : TBase(param)
    {
    }

    template <class TOwner>
    static NPointValueBinders::TOwnerBinder<TBinderData<TOwner>> Bind(TOwner* owner) {
        return NPointValueBinders::TOwnerBinder<TBinderData<TOwner>>(owner);
    }
};
