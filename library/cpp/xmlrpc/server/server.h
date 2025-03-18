#pragma once

#include "wrap.h"

#include <library/cpp/neh/rpc.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NXmlRPC {
    class IServer: public NNeh::IService {
    public:
        template <class F>
        inline IServer& Add(const TString& funcname, F f) {
            this->Add(funcname, ToXmlRPCCallback(f));

            return *this;
        }

    private:
        virtual void Add(const TString& funcname, TXmlRPCCallback cb) = 0;
    };

    typedef TIntrusivePtr<IServer> IServerRef;

    IServerRef CreateServer();
}
