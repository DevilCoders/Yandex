#pragma once

#include "copy_points.h"
#include "slave_misc_points.h"

#include <utility>
#include <util/system/type_name.h>
#include <util/string/vector.h>

#include <typeinfo>

template <class... A>
class TMasterNArgsPoint: public TMasterCopyPoint<void (*)(void*, A...)> {
private:
    typedef void (*TNArgsFunction)(void*, A...);
    typedef TMasterCopyPoint<TNArgsFunction> TBase;

protected:
    /// @throws TIncompatibleAccessPoints
    void CheckCompatibility(const TSlaveAccessPoint& slaveAccessPoint) const override {
        if (typeid(slaveAccessPoint) != typeid(TSlaveNArgsPoint<A...>))
            ythrow TIncompatibleAccessPoints() << TypeName<TMasterNArgsPoint<A...>>() + ": Connecting not compatible access points: " << TypeName(slaveAccessPoint) << " != " << TypeName<TSlaveNArgsPoint<A...>>();
    }

public:
    TMasterNArgsPoint()
        : TBase(&DefaultFunc)
    {
    }
    TMasterNArgsPoint(TSimpleModule* module, const TString& names)
        : TBase(module, &DefaultFunc, names)
    {
    }

    void Func(A... a) const {
        try {
            TBase::Val(TBase::GetOwner(), a...);
        } catch (const TUninitModuleException&) {
            TBase::ThrowUninitException(TypeName<TMasterNArgsPoint<A...>>());
        }
    }

private:
    static void DefaultFunc(void*, A...) {
        ythrow TUninitModuleException();
    }
};
template <class A>
using TMasterArgsPoint = TMasterNArgsPoint<A>;
template <class A, class B>
using TMaster2ArgsPoint = TMasterNArgsPoint<A, B>;
template <class A, class B, class C>
using TMaster3ArgsPoint = TMasterNArgsPoint<A, B, C>;

template <class B, class... A>
class TMasterAnswerNPoint: public TMasterNArgsPoint<A..., B&> {
private:
    typedef TMasterNArgsPoint<A..., B&> TBase;

public:
    TMasterAnswerNPoint() {
    }
    TMasterAnswerNPoint(TSimpleModule* module, const TString& names)
        : TBase(module, names)
    {
    }

    B Answer(A... a) const {
        B b;
        try {
            TBase::Val(TBase::GetOwner(), a..., b);
        } catch (const TUninitModuleException&) {
            TBase::ThrowUninitException(TypeName<TMasterAnswerNPoint<B, A...>>());
        }
        return b;
    }
};
template <class A, class B>
using TMasterAnswerPoint = TMasterAnswerNPoint<B, A>;
template <class A1, class A2, class B>
using TMasterAnswer2Point = TMasterAnswerNPoint<B, A1, A2>;
