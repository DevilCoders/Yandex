#include "get_host_name_request.h"

#include "bad_request_handler.h"
#include "environment.h"
#include "instance_hashing.h"
#include "request_context.h"
#include "request_params.h"

#include <antirobot/idl/ip2backend.pb.h>
#include <antirobot/lib/ar_utils.h>
#include <antirobot/lib/keyring.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/http/io/stream.h>

#include <util/generic/xrange.h>
#include <util/generic/yexception.h>
#include <util/network/address.h>
#include <util/system/hostname.h>

#include <utility>

namespace NAntiRobot {

const TString COMMAND_GET_HOST_NAME("/getarhost");

namespace {
const TString CGI_REQUESTER_IP("requester");
const TString CGI_ASKED_IP("asked");
const TString CGI_PROCESSOR_COUNT("p");
const TString HEADER_X_REAL_IP("X-Real-Ip");

std::pair<TAddr, TAddr> ParseGetHostNameRequest(const TRequest& request) {
    const TCgiParameters& cgi = request.CgiParams;

    if (request.Headers.Get(HEADER_X_REAL_IP) != cgi.Get(CGI_ASKED_IP)) {
        ythrow yexception() << "Header and CGI IPs are different: "
                            << request.Headers.Get(HEADER_X_REAL_IP)
                            << ", " << cgi.Get(CGI_ASKED_IP);
    }
    TAddr asked(cgi.Get(CGI_ASKED_IP));
    if (asked == TAddr()) {
        ythrow yexception() << "Failed to parse asked IP: " << cgi.Get(CGI_ASKED_IP);
    }

    TAddr requester(cgi.Get(CGI_REQUESTER_IP));
    if (requester == TAddr()) {
        ythrow yexception() << "Failed to parse requester IP: " << cgi.Get(CGI_REQUESTER_IP);
    }
    return {asked, requester};
}

}

THttpRequest CreateGetHostNameRequest(const TString& host, const TString& requestedIP, int processorCount) {
    TNetworkAddress thisAddr(HostName(), 80);
    NAddr::TAddrInfo ip4or6(&*thisAddr.Begin());

    TCgiParameters cgi;
    cgi.InsertUnescaped(CGI_REQUESTER_IP, NAddr::PrintHost(ip4or6));
    cgi.InsertUnescaped(CGI_ASKED_IP, requestedIP);
    cgi.InsertUnescaped(CGI_PROCESSOR_COUNT, ToString(processorCount));

    auto result = HttpGet(host, COMMAND_GET_HOST_NAME);
    result.AddCgiParameters(cgi);
    result.AddHeader(HEADER_X_REAL_IP, requestedIP);
    result.AddHeader("X-Forwarded-For-Y", requestedIP);
    return result;
}

NThreading::TFuture<TResponse> HandleGetHostNameRequest(TRequestContext& rc) {
    try {
        const auto [asked, requester] = ParseGetHostNameRequest(*rc.Req);

        if (!rc.Env.ReloadableData.ServiceToYandexIps[rc.Req->HostType].Get()->Contains(requester)) {
            return TBadRequestHandler(HTTP_BAD_REQUEST)(rc);
        }

        NIp2BackendProto::TResponse response;
        response.SetCacher(HostName());

        const auto processorCount = FromString<size_t>(rc.Req->CgiParams.Get(CGI_PROCESSOR_COUNT));
        const auto processors = rc.Env.BackendSender.GetSequence(
            rc.Env.CustomHashingMap,
            asked,
            processorCount,
            rc.Env.DisablingFlags.IsStopDiscoveryForAll()
        );

        *response.MutableProcessors() = {processors.begin(), processors.end()};

        return NThreading::MakeFuture(TResponse::ToUser(HTTP_OK).SetContent(response.SerializeAsString()));
    } catch (...) {
        return TBadRequestHandler(HTTP_BAD_REQUEST)(rc);
    }
}

} /* namespace NAntiRobot */
