#pragma once

#include <library/cpp/xmlrpc/protocol/value.h>
#include <library/cpp/xmlrpc/protocol/rpcfault.h>
#include <util/generic/function.h>

namespace NXmlRPC {
    typedef std::function<TValue(const TArray&)> TXmlRPCCallback;

    template <size_t N, class F>
    struct TCall;

#include <library/cpp/xmlrpc/server/genwrap.h>

    template <class F>
    static inline TXmlRPCCallback ToXmlRPCCallback(F f) {
        return std::bind(TCall<TFunctionArgs<F>::Length, F>::Call, f, std::placeholders::_1);
    }
}
