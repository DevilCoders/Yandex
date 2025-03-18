#include "req2info.h"

#include <antirobot/daemon_lib/reqinfo_request.h>
#include <antirobot/lib/addr.h>
#include <antirobot/lib/http_helpers.h>
#include <antirobot/lib/http_request.h>


NAntiRobot::THttpRequest MakeRequest(const TString& host, bool asFullReq, const TString& requestToInspect) {
    auto req = NAntiRobot::CreateReqInfoRequest(TString(host), requestToInspect);

    if (!asFullReq) {
        return req;
    }

    auto fullReq = NAntiRobot::HttpPost(TString(host), "/fullreq");
    fullReq.SetContent(ToString(req));

    return fullReq;
}

TString GetReqInfo(const TString& hostPort, bool asFullReq, const TString& requestToInspect) {
    auto reqInfo = MakeRequest(hostPort, asFullReq, requestToInspect);
    static NAntiRobot::TNehRequester nehRequester(10000);
    return NAntiRobot::FetchHttpDataUnsafe(&nehRequester, NAntiRobot::CreateHostAddr(hostPort), reqInfo, TDuration::Seconds(10), "http");
}
