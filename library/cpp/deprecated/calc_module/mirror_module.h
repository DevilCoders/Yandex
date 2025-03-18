#pragma once

#include "simple_module.h"

template <class A, class B = A>
class TMirrorModule: public TSimpleModule {
private:
    TMirrorModule()
        : TSimpleModule("TMirrorModule")
    {
        Bind(this).template To<A, B&, &TMirrorModule::Reply>("reply");
    }

public:
    static TCalcModuleHolder BuildModule() {
        return new TMirrorModule();
    }

private:
    void Reply(A a, B& b) {
        b = a;
    }
};
