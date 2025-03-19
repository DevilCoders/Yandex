#pragma once

#include "request_contents.h"
#include "request_accessors.h"

namespace NReqBundle {
    class TRequest
        : public TAtomicRefCount<TRequest>
        , public TRequestAcc
    {
    private:
        NDetail::TRequestData Data;

    public:
        TRequest()
            : TRequestAcc(Data)
        {
        }
        TRequest(const NDetail::TRequestData& data)
            : TRequestAcc(Data)
            , Data(data)
        {}
        TRequest(TConstRequestAcc other)
            : TRequest(NDetail::BackdoorAccess(other))
        {}
        TRequest(const TRequest& other)
            : TRequest(other.Data)
        {}

        TRequest& operator=(const TRequest& other) {
            Data = other.Data;
            return *this;
        }
    };

    using TRequestPtr = TIntrusivePtr<TRequest>;
} // NReqBundle
