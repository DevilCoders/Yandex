#pragma once

#include "access_points.h"
#include "value_binders.h"

class TUninitModuleException: public yexception {
};

template <class T>
class TSlaveCopyPoint: public TSlaveAccessPoint {
private:
    const T Val;

    template <class TOwnerType>
    class TBinderData {
    public:
        typedef TOwnerType TOwner;
        typedef T TValue;
    };

protected:
    template <class TConcreteBinderData>
    TSlaveCopyPoint(NPointValueBinders::TOwnerValueBinder<TConcreteBinderData> param)
        : TSlaveAccessPoint(param.GetOwner())
        , Val(param.GetValue())
    {
        RegisterMe(param.GetOwner(), param.GetNames());
    }

public:
    TSlaveCopyPoint(TSimpleModule* module, T val, const TString& names)
        : TSlaveAccessPoint(module)
        , Val(val)
    {
        RegisterMe(module, names);
    }

    TSlaveCopyPoint(void* owner, T val)
        : TSlaveAccessPoint(owner)
        , Val(val)
    {
    }

    T GetValue() const noexcept {
        return Val;
    }
};
