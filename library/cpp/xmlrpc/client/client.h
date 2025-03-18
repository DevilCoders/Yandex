#pragma once

#include <library/cpp/xmlrpc/protocol/value.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/datetime/base.h>

namespace NXmlRPC {
    struct TXmlRPCRemoteError: public TXmlRPCError {
    };

    class IHandle: public TThrRefBase {
    public:
        ~IHandle() override;

        virtual const TValue& Wait(TInstant deadline) = 0;

        inline const TValue& Wait(TDuration d) {
            return Wait(d.ToDeadLine());
        }

        inline const TValue& Wait() {
            return Wait(TInstant::Max());
        }
    };

    typedef TIntrusivePtr<IHandle> IHandleRef;

    struct TEndPoint {
        TString Url;

        inline TEndPoint(const TString& url)
            : Url(url)
        {
        }

        IHandleRef SendRequest(const TStringBuf& func, const TArray& params);

        template <typename... R>
        inline IHandleRef AsyncCall(const TStringBuf& func, const R&... r) {
            return SendRequest(func, TArray(r...));
        }
    };
}
