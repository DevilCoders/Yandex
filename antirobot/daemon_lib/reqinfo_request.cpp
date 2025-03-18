#include "reqinfo_request.h"

#include "bad_request_handler.h"
#include "request_context.h"
#include "request_params.h"
#include "environment.h"

#include <antirobot/lib/ar_utils.h>
#include <antirobot/lib/keyring.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/http/io/stream.h>

#include <util/generic/yexception.h>
#include <util/system/hostname.h>

namespace NAntiRobot {

const TString COMMAND_GETREQINFO("/getreqinfo");

THttpRequest CreateReqInfoRequest(const TString& host, const TString& rawreq) {
    ui32 thisHostIP;
    NResolver::GetHostIP(HostName().data(), &thisHostIP);

    auto result = HttpPost(host, COMMAND_GETREQINFO);
    result.AddHeader("X-Forwarded-For-Y", IpToStr(thisHostIP));
    result.SetContent(rawreq);

    return result;
}

NThreading::TFuture<TResponse> HandleReqInfoRequest(TRequestContext& rc) {
    try {
        TRequestContext embedReq = ExtractWrappedRequest(rc);
        const TRequest& req = *embedReq.Req;
#define PAR(field) #field << ": " << req.field << "\n"
        TString s;
        TStringOutput str(s);

        str << "UserAddr: " << req.UserAddr <<
               " (IsYandex: " << req.UserAddr.IsYandex() << ", IsPrivileged: " << req.UserAddr.IsPrivileged() << ", IsWhitelisted: " << req.UserAddr.IsWhitelisted() << ")" << "\n"
            << PAR(RequesterAddr)
            << PAR(PartnerAddr)
            << PAR(ClientType)
            << PAR(Uid)
            << PAR(SpravkaAddr)
            << PAR(SpravkaTime)
            << PAR(HasValidFuid)
            << PAR(HasValidLCookie)
            << PAR(HasValidSpravka)
            << PAR(ReqType)
            << "ReqGroup: " << rc.Env.ReqGroupClassifier.GroupName(req.HostType, req.ReqGroup) << '\n'
            << PAR(HostType)
            << PAR(CaptchaReqType)
            << PAR(BlockCategory)
            << PAR(YandexUid)
            << PAR(IsSearch)
            << PAR(InitiallyWasXmlsearch)
            << PAR(ForceShowCaptcha)
            << PAR(IsPartnerRequest())
            << PAR(CanShowCaptcha())
            << PAR(MayBanFor())
            << PAR(IsAccountableRequest())
            << PAR(IsImportantRequest())
            << PAR(IsNewsClickReq())
            << PAR(IsMetricalReq())
            << "MatrixnetFormulaThreshold: "
            << ANTIROBOT_DAEMON_CONFIG.JsonConfig.ParamsByTld(req.HostType, req.Tld).ProcessorThreshold << "\n"
            << Endl;
#undef PAR
        return NThreading::MakeFuture(TResponse::ToUser(HTTP_OK).SetContent(s));
    } catch (...) {
        Cerr << "Failed to generate ReqInfo: " << CurrentExceptionMessage() << Endl;
        return TBadRequestHandler(HTTP_BAD_REQUEST)(rc);
    }
}

} /* namespace NAntiRobot */
