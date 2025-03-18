#include "client.h"

#include <library/cpp/xmlrpc/protocol/protocol.h>
#include <library/cpp/xmlrpc/protocol/compress.h>

#include <library/cpp/neh/neh.h>
#include <library/cpp/neh/location.h>

#include <util/stream/output.h>
#include <util/string/cast.h>

using namespace NXmlRPC;

namespace {
    struct TRPCHandle: public IHandle {
        inline TRPCHandle(const NNeh::THandleRef& h)
            : H(h)
        {
        }

        const TValue& Wait(TInstant deadline) override {
            if (!V) {
                NNeh::TResponseRef ret = H->Wait(deadline);

                if (!ret) {
                    ythrow TXmlRPCRemoteError() << "Timeout";
                }

                if (ret->IsError()) {
                    ythrow TXmlRPCRemoteError() << ret->GetErrorText();
                }

                const TString data = ret->Data;

                if (!data) {
                    ythrow TXmlRPCRemoteError() << "empty data in response";
                }

                V.Reset(new TValue(ParseResult(data)));
            }

            return *V;
        }

        NNeh::THandleRef H;
        THolder<TValue> V;
    };
}

IHandle::~IHandle() {
}

IHandleRef TEndPoint::SendRequest(const TStringBuf& func, const TArray& params) {
    using namespace NNeh;

    const TString data = SerializeRequest(func, params);

    //extra copy here
    {
        TParsedLocation loc(Url);

        if (loc.Scheme == TStringBuf("http")) {
            TStringStream ss;

            ss << TStringBuf("POST /") << loc.Service << TStringBuf(" HTTP/1.1\r\nHost: ") << loc.EndPoint
               << TStringBuf("\r\nContent-Type: text/xml\r\nContent-Length: ") << data.size() << TStringBuf("\r\n\r\n") << data;

            return new TRPCHandle(NNeh::Request(NNeh::TMessage(TString("full://") + loc.EndPoint + "/" + loc.Service, ss.Str())));
        }
    }

    return new TRPCHandle(NNeh::Request(NNeh::TMessage(Url, CompressXml(data))));
}
