#pragma once

#include "errors.h"
#include "value.h"

namespace NXmlRPC {
    class TXmlRPCFault: public TXmlRPCError {
    public:
        TXmlRPCFault(const TValue& v);

        inline const TValue& Fault() const noexcept {
            return V_;
        }

        int Code() const;
        TString String() const;

    private:
        TValue V_;
    };
}
