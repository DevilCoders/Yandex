#include "http_helpers.h"

#include "neh_requester.h"

#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/neh/http_common.h>
#include <library/cpp/threading/future/future.h>

#include <util/network/endpoint.h>

namespace NAntiRobot {

THttpCodeException::THttpCodeException(ui32 code)
    : Code(code)
{
    *this << Code << " (" << HttpCodeStr(Code) << ") ";
}

NThreading::TFuture<TErrorOr<NNeh::TResponseRef>> FetchHttpDataAsync(
    TNehRequester* nehRequester,
    const TNetworkAddress& addr,
    const THttpRequest& req,
    const TDuration& timeout,
    const TString& protocol)
{
    const TString& request = req.CreateUrl(addr, protocol);
    NNeh::TMessage msg = NNeh::TMessage::FromString(request);

    auto requestType = FromString<NNeh::NHttp::ERequestType>(req.GetMethod());
    NNeh::NHttp::MakeFullRequest(msg, req.GetHeadersAsString(), req.GetContent(), "", requestType);
    return nehRequester->RequestAsync(msg, timeout);
}

TString FetchHttpDataUnsafe(
    TNehRequester* nehRequester,
    const TNetworkAddress& addr,
    const THttpRequest& req,
    const TDuration& timeout,
    const TString& protocol)
{
    auto responseFuture = FetchHttpDataAsync(nehRequester,  addr, req, timeout, protocol);
    NNeh::TResponseRef response;

    if (TError err = responseFuture.GetValueSync().PutValueTo(response); err.Defined()) {
        err.Throw();
    }

    if (!response->IsError()) {
        return response->Data;
    } else {
        if (!response) {
            ythrow yexception() << "null response";
        }
        ythrow THttpCodeException(static_cast<ui32>(response->GetErrorCode())) << addr << ", " << response->GetErrorText();
    }
}

}
