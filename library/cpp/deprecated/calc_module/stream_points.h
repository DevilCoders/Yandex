#pragma once

#include "copy_points.h"
#include "slave_stream_points.h"

#include <util/system/type_name.h>

template <class T>
class TMasterOutputPoint: public TMasterCopyPoint<void (*)(void*, T)> {
private:
    typedef void (*TCopyFunction)(void*, T);
    typedef TMasterCopyPoint<TCopyFunction> TBase;

protected:
    /// @throws TIncompatibleAccessPoints
    void CheckCompatibility(const TSlaveAccessPoint& slaveAccessPoint) const override {
        if (typeid(slaveAccessPoint) != typeid(TSlaveInputPoint<T>))
            ythrow TIncompatibleAccessPoints() << "TMasterOutputPoint: Connecting not compatible access points: " << TypeName(slaveAccessPoint) << " != " << TypeName<TSlaveInputPoint<T>>();
    }

public:
    TMasterOutputPoint()
        : TBase(&DefaultWrite)
    {
    }
    TMasterOutputPoint(TSimpleModule* module, const TString& names)
        : TBase(module, &DefaultWrite, names)
    {
    }

    void Write(T val) {
        try {
            TBase::Val(TBase::GetOwner(), val);
        } catch (const TUninitModuleException&) {
            TBase::ThrowUninitException("TMasterOutputPoint");
        }
    }

private:
    static void DefaultWrite(void*, T) {
        ythrow TUninitModuleException();
    }
};

template <class T>
class TMasterInputPoint: public TMasterCopyPoint<T (*)(void*)> {
private:
    typedef T (*TCopyFunction)(void*);
    typedef TMasterCopyPoint<TCopyFunction> TBase;

protected:
    /// @throws TIncompatibleAccessPoints
    void CheckCompatibility(const TSlaveAccessPoint& slaveAccessPoint) const override {
        if (typeid(slaveAccessPoint) != typeid(TSlaveOutputPoint<T>))
            ythrow TIncompatibleAccessPoints() << "TMasterInputPoint: Connecting not compatible access points: " << TypeName(slaveAccessPoint) << " != " << TypeName<TSlaveOutputPoint<T>>();
    }

public:
    TMasterInputPoint()
        : TBase(&DefaultRead)
    {
    }
    TMasterInputPoint(TSimpleModule* module, const TString& names)
        : TBase(module, &DefaultRead, names)
    {
    }

    T Read() {
        try {
            return TBase::Val(TBase::GetOwner());
        } catch (const TUninitModuleException&) {
            TBase::ThrowUninitException("TMasterInputPoint");
            throw yexception(); // Suppressing compiling warning messages. In fact don't used.
        }
    }

private:
    static T DefaultRead(void*) {
        ythrow TUninitModuleException();
    }
};
