#pragma once

#include <util/thread/pool.h>
#include <util/generic/ptr.h>
#include <util/generic/typetraits.h>

#include <functional>

template <class TFunc>
class TFunctionMtpJob: public IObjectInQueue {
public:
    template <class TFuncType>
    TFunctionMtpJob(TFuncType&& f)
        : Func(std::forward<TFuncType>(f))
    {
    }

    void Process(void* threadSpecificResource) override {
        CallFunc(threadSpecificResource,
                 std::integral_constant<bool, std::is_invocable_v<TFunc, void*>>());
    }

private:
    void CallFunc(void*, std::false_type) {
        Func();
    }

    void CallFunc(void* threadSpecificResource, std::true_type) {
        Func(threadSpecificResource);
    }

private:
    TFunc Func;
};

template <class TFunc>
TAutoPtr<IObjectInQueue> MakeFunctionMtpJob(TFunc&& f) {
    // for lvalue: copy of f is stored internally
    // for rvalue: f is moved inside
    using TJob = TFunctionMtpJob<std::remove_reference_t<TFunc>>;
    return new TJob(std::forward<TFunc>(f));
}
