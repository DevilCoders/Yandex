#pragma once

#include "simple_module.h"
#include "stream_points.h"

template <class A, class B = A>
class TStreamAdapter: public TSimpleModule {
private:
    TMasterInputPoint<A> InputPoint;
    TMasterOutputPoint<B> OutputPoint;

    TStreamAdapter()
        : TSimpleModule("TStreamAdapter")
        , InputPoint(this, "input")
        , OutputPoint(this, "output")
    {
        Bind(this).template To<&TStreamAdapter::Transfer>("transfer");
        AddPointDependencies("transfer", "input,output");
    }

public:
    static TCalcModuleHolder BuildModule() {
        return new TStreamAdapter();
    }

private:
    void Transfer() {
        OutputPoint.Write(InputPoint.Read());
    }
};
