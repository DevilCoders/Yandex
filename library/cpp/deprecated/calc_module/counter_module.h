#pragma once

#include "stream_points.h"
#include "simple_module.h"

#include <library/cpp/deprecated/atomic/atomic.h>

namespace NCounterModule {
    template <typename TValue>
    void AddToValue(TValue& sum, const TValue& value) {
        sum += value;
    }
    void AddToValue(TAtomic& sum, const TAtomic& value) {
        AtomicAdd(sum, value);
    }
}

template <typename TValue = ui64>
class TCounterModule: public TSimpleModule {
private:
    TValue Counter;

    TCounterModule()
        : TSimpleModule("TCounterModule")
        , Counter()
    {
        Bind(this).template To<TValue, &TCounterModule::Add>("input");
        Bind(this).template To<TValue, &TCounterModule::Get>("output");
    }

public:
    static TCalcModuleHolder BuildModule() {
        return new TCounterModule;
    }

private:
    void Add(TValue value) {
        NCounterModule::AddToValue(Counter, value);
    }
    TValue Get() {
        return Counter;
    }
};
