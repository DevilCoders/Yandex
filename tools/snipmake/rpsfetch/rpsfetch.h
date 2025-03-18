#pragma once

#include <library/cpp/neh/neh.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>

namespace NSnippets {
    /*
     * use NNeh::IOnRecv callback in THandle c-tor and it'll be called in THandle d-tor after fetching
     * this'll happen synchronously on Add and WaitAll calls when some fetches are complete
     * optimized for dense Adds, for sparse Adds may waste available rps due to optimizations
     */
    class TPoliteFetcher {
    public:
        TPoliteFetcher(int rps, int maxInFlight);
        ~TPoliteFetcher();

        bool Add(const NNeh::THandleRef& req, const TInstant& addBefore = TInstant::Now());
        bool WaitAll(const TInstant& deadline = TInstant::Max());

    private:
        struct TImpl;
        THolder<TImpl> Impl;
    };
}
