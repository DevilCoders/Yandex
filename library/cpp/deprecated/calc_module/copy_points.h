#pragma once

#include "slave_copy_points.h"

#include <util/system/type_name.h>

template <class T>
class TMasterCopyPoint: public TMasterAccessPoint {
private:
    template <class TOwnerType>
    class TBinderData {
    public:
        typedef TOwnerType TOwner;
        typedef T TValue;
    };

protected:
    T Val;
    T DefaultVal;

    /// @throws TIncompatibleAccessPoints
    void CheckCompatibility(const TSlaveAccessPoint& slaveAccessPoint) const override {
        if (typeid(slaveAccessPoint) != typeid(TSlaveCopyPoint<T>)) {
            ythrow TIncompatibleAccessPoints() << "TMasterCopyPoint: Connecting not compatible access points: " << TypeName(slaveAccessPoint) << " != " << TypeName<TSlaveCopyPoint<T>>();
        }
    }
    void DoConnect(const TSlaveAccessPoint& slaveAccessPoint) override {
        Val = ((TSlaveCopyPoint<T>&)slaveAccessPoint).GetValue();
    }
    void DoDrop() override {
        Val = DefaultVal;
    }

public:
    TMasterCopyPoint(TSimpleModule* module, T val, const TString& names)
        : Val(val)
        , DefaultVal(val)
    {
        RegisterMe(module, names);
    }

    TMasterCopyPoint(TSimpleModule* module, const TString& names)
        : Val()
        , DefaultVal()
    {
        RegisterMe(module, names);
    }

    TMasterCopyPoint(T val)
        : Val(val)
        , DefaultVal(val)
    {
    }

    T GetValue() const {
        if (!GetConnectionId())
            ThrowUninitException("TMasterCopyPoint");
        return Val;
    }
};
