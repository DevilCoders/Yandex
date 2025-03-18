// ====================================================================
//  Implementation of main blackbox interface functions
// ====================================================================

#include "responseimpl.h"
#include "utils.h"

#include <library/cpp/blackbox2/blackbox2.h>

#include <util/string/cast.h>

namespace NBlackbox2 {
    // ==== Response structure ============================================

    TResponse::TResponse(THolder<TImpl> pImpl)
        : Impl_(std::move(pImpl))
        , Message_()
    {
        if (!Impl_.Get())
            throw TFatalError(NSessionCodes::UNKNOWN,
                              "Internal error: NULL Response::Impl");

        // empty if no <error> tag
        Impl_->GetIfExists("error", Message_);

        long int err_code;
        if (Impl_->GetIfExists("exception/@id", err_code)) {
            NSessionCodes::ESessionError code = static_cast<NSessionCodes::ESessionError>(err_code);
            if (code == NSessionCodes::DB_EXCEPTION || code == NSessionCodes::DB_FETCHFAILED)
                throw TTempError(code, Message_);
            else
                throw TFatalError(code, Message_);
        }
    }

    TResponse::~TResponse() {
    }

    TBulkResponse::TBulkResponse(THolder<TImpl> pImpl)
        : TResponse(std::move(pImpl))
    {
        xmlConfig::Parts users = Impl_->GetParts("user");
        Count_ = users.Size();
    }

    TBulkResponse::~TBulkResponse() {
    }

    TString TBulkResponse::Id(int i) const {
        if (i < 0 || i >= Count_)
            throw TFatalError(NSessionCodes::INVALID_PARAMS,
                              "Bulk response index out of range");

        xmlConfig::Part user = Impl_->GetParts("user")[i];

        TString id;
        user.GetIfExists("@id", id);

        return id;
    }

    THolder<TResponse> TBulkResponse::User(int i) const {
        if (i < 0 || i >= Count_)
            throw TFatalError(NSessionCodes::INVALID_PARAMS,
                              "Bulk response index out of range");

        xmlConfig::Part user = Impl_->GetParts("user")[i];

        TResponse* resp = new TResponse(THolder<TResponse::TImpl>(
            new TResponse::TImpl(&user)));

        return THolder<TResponse>(resp);
    }

    TSessionResp::TSessionResp(THolder<TImpl> pImpl)
        : TResponse(std::move(pImpl))
    {
        long int status;
        if (!Impl_->GetIfExists("status/@id", status))
            throw TFatalError(NSessionCodes::UNKNOWN,
                              "Bad blackbox response: missing status/@id");
        Status_ = static_cast<EStatus>(status);

        // empty if no <age> found
        Impl_->GetIfExists("age", Age_);

        long int uid;
        IsLite_ = Impl_->GetIfExists("liteuid", uid);
    }

    TSessionResp::~TSessionResp() {
    }

    TMultiSessionResp::TMultiSessionResp(THolder<TImpl> pImpl)
        : TResponse(std::move(pImpl))
    {
        long int status;
        if (!Impl_->GetIfExists("status/@id", status))
            throw TFatalError(NSessionCodes::UNKNOWN,
                              "Bad blackbox response: missing status/@id");
        Status_ = static_cast<TSessionResp::EStatus>(status);

        // empty if no <age> found
        Impl_->GetIfExists("age", Age_);

        Impl_->GetIfExists("default_uid", DefaultUid_);

        xmlConfig::Parts users = Impl_->GetParts("user");
        Count_ = users.Size();
    }

    TMultiSessionResp::~TMultiSessionResp() {
    }

    TString TMultiSessionResp::Id(int i) const {
        if (i < 0 || i >= Count_)
            throw TFatalError(NSessionCodes::INVALID_PARAMS,
                              "Multisession response index out of range");

        xmlConfig::Part user = Impl_->GetParts("user")[i];

        TString id;
        user.GetIfExists("@id", id);

        return id;
    }

    THolder<TSessionResp> TMultiSessionResp::User(int i) const {
        if (i < 0 || i >= Count_)
            throw TFatalError(NSessionCodes::INVALID_PARAMS,
                              "Multisession response index out of range");

        xmlConfig::Part user = Impl_->GetParts("user")[i];

        TSessionResp* resp = new TSessionResp(THolder<TResponse::TImpl>(
            new TResponse::TImpl(&user)));

        return THolder<TSessionResp>(resp);
    }

    TLoginResp::TLoginResp(THolder<TImpl> pImpl)
        : TResponse(std::move(pImpl))
        , AccStatus_(accUnknown)
        , PwdStatus_(pwdUnknown)
    {
        long int status;
        if (!Impl_->GetIfExists("status/@id", status)) {
            // Try login v2 fields
            long int astatus, pstatus;

            if (!Impl_->GetIfExists("login_status/@id", astatus) ||
                !Impl_->GetIfExists("password_status/@id", pstatus))
                throw TFatalError(NSessionCodes::UNKNOWN,
                                  "Bad blackbox response: missing login status");
            AccStatus_ = static_cast<EAccountStatus>(astatus);
            PwdStatus_ = static_cast<EPasswdStatus>(pstatus);

            // set up legacy v1 status field
            TString tmp;
            if (AccStatus_ == accValid && PwdStatus_ == pwdValid)
                Status_ = Valid;
            else if (AccStatus_ == accDisabled)
                Status_ = Disabled;
            else if (AccStatus_ == accAlienDomain)
                Status_ = AlienDomain;
            else if (PwdStatus_ == pwdCompromised)
                Status_ = Compromised;
            else if (Impl_->GetIfExists("bruteforce_policy/captcha", tmp))
                Status_ = ShowCaptcha;
            else if (Impl_->GetIfExists("bruteforce_policy/password_expired", tmp))
                Status_ = Expired;
            else
                Status_ = Invalid;
        } else
            Status_ = static_cast<EStatus>(status);

        TString comment;
        Impl_->GetIfExists("comment", comment);
        if (!comment.empty())
            Message_ = comment;
    }

    TLoginResp::~TLoginResp() {
    }

    // ==== Blackbox interface methods ====================================

    // method = userinfo
    //     THolder<Response> Info (const TString& uid,
    //         const TString& userIp, const Options& options)
    //     {
    //        TString req = InfoRequest(uid, userIp, options);
    //
    //        TString resp;
    //         Server::instance().Request(req, resp);
    //
    //         return InfoResponse(resp);
    //     }
    //
    //
    //     THolder<Response> Info (const LoginSid& loginsid,
    //         const TString& userIp, const Options& options)
    //     {
    //        TString req = InfoRequest(loginsid, userIp, options);
    //
    //        TString resp;
    //         Server::instance().Request(req, resp);
    //
    //         return InfoResponse(resp);
    //     }
    //
    //
    //     THolder<Response> Info (const SuidSid& suidsid,
    //         const TString& userIp, const Options& options)
    //     {
    //        TString req = InfoRequest(suidsid, userIp, options);
    //
    //        TString resp;
    //         Server::instance().Request(req, resp);
    //
    //         return InfoResponse(resp);
    //     }
    //
    //
    //     // method = login
    //     THolder<LoginResp> Login (const LoginSid& loginsid,
    //         const TString& password, const TString& from,
    //         const TString& userIp, const Options& options)
    //     {
    //         LoginReqData req = LoginRequest(loginsid, password, from,
    //                                        userIp, options);
    //
    //        TString resp;
    //         Server::instance().Request(req.uri_, req.postData_, resp);
    //
    //         return LoginResponse(resp);
    //     }
    //
    //
    //     // method = sessionid
    //     THolder<SessionResp> SessionID (const TString& sessionId,
    //         const TString& hostname,
    //         const TString& userIp, const Options& options)
    //     {
    //        TString req = SessionIDRequest(sessionId, hostname, userIp, options);
    //
    //        TString resp;
    //         Server::instance().Request(req, resp);
    //
    //         return SessionIDResponse(resp);
    //     }

    // ==== Common constants ==============================================

    const TString strMethodlogin("method=login");
    const TString strMethodSessionid("method=sessionid");
    const TString strMethodOAuth("method=oauth");
    const TString strMethodUserinfo("method=userinfo");

    // ==== Blackbox interface without network operations =================

    // method = userinfo
    TString InfoRequest(const TStringBuf uid, const TStringBuf userIp, const TOptions& options) {
        TRequestBuilder req(strMethodUserinfo);
        req.Add("uid", uid);
        req.Add("userip", userIp);
        req.Add(options);

        return req;
    }

    TString InfoRequest(const TLoginSid& loginsid, const TStringBuf userIp, const TOptions& options) {
        TRequestBuilder req(strMethodUserinfo);
        TString punycode;
        const TString& ref = NIdn::EncodeAddr(loginsid.Login, punycode);
        req.Add("login", ref);
        if (!loginsid.Sid.empty())
            req.Add("sid", loginsid.Sid);
        req.Add("userip", userIp);
        req.Add(options);

        return req;
    }

    TString InfoRequest(const TSuidSid& suidsid, const TStringBuf userIp, const TOptions& options) {
        TRequestBuilder req(strMethodUserinfo);
        req.Add("suid", suidsid.Suid);
        req.Add("sid", suidsid.Sid);
        req.Add("userip", userIp);
        req.Add(options);

        return req;
    }

    TString InfoRequest(const TVector<TString>& uids, const TStringBuf userIp, const TOptions& options) {
        TRequestBuilder req(strMethodUserinfo);
        req.Add("uid", uids);
        req.Add("userip", userIp);
        req.Add(options);

        return req;
    }

    THolder<TResponse> InfoResponse(const TStringBuf bbResponse) {
        TResponse* resp = new TResponse(THolder<TResponse::TImpl>(
            new TResponse::TImpl(bbResponse)));

        return THolder<TResponse>(resp);
    }

    THolder<TBulkResponse> InfoResponseBulk(const TStringBuf bbResponse) {
        TBulkResponse* resp = new TBulkResponse(THolder<TResponse::TImpl>(
            new TResponse::TImpl(bbResponse)));

        return THolder<TBulkResponse>(resp);
    }

    // method = login
    TLoginReqData LoginRequest(const TLoginSid& loginsid,
                               const TStringBuf password, const TStringBuf authtype,
                               const TStringBuf userIp, const TOptions& options) {
        TLoginReqData reqData;

        TRequestBuilder req(strMethodlogin);
        TString punycode;
        const TString& ref = NIdn::EncodeAddr(loginsid.Login, punycode);
        req.Add("login", ref);
        if (!loginsid.Sid.empty())
            req.Add("sid", loginsid.Sid);
        req.Add("authtype", authtype);
        req.Add("userip", userIp);
        req.Add(options);

        reqData.Uri_ = req;

        reqData.PostData_.reserve(password.size() * 2);
        reqData.PostData_.append("password=").append(URLEncode(password));

        return reqData;
    }

    TLoginReqData LoginRequestUid(const TStringBuf uid,
                                  const TStringBuf password, const TStringBuf authtype,
                                  const TStringBuf userIp, const TOptions& options) {
        TLoginReqData reqData;

        TRequestBuilder req(strMethodlogin);
        req.Add("uid", uid);
        req.Add("authtype", authtype);
        req.Add("userip", userIp);
        req.Add(options);

        reqData.Uri_ = req;

        reqData.PostData_.reserve(password.size() * 2);
        reqData.PostData_.append("password=").append(URLEncode(password));

        return reqData;
    }

    THolder<TLoginResp> LoginResponse(const TStringBuf bbResponse) {
        TLoginResp* resp = new TLoginResp(THolder<TResponse::TImpl>(
            new TResponse::TImpl(bbResponse)));

        return THolder<TLoginResp>(resp);
    }

    // method = sessionid
    TString SessionIDRequest(const TStringBuf sessionId,
                             const TStringBuf hostname,
                             const TStringBuf userIp,
                             const TOptions& options) {
        TRequestBuilder req(strMethodSessionid);
        req.Add("sessionid", sessionId);
        req.Add("host", hostname);
        req.Add("userip", userIp);
        req.Add(options);

        return req;
        ;
    }

    THolder<TSessionResp> SessionIDResponse(const TStringBuf bbResponse) {
        return MakeHolder<TSessionResp>(MakeHolder<TResponse::TImpl>(bbResponse));
    }

    THolder<TMultiSessionResp> SessionIDResponseMulti(const TStringBuf bbResponse) {
        TMultiSessionResp* resp = new TMultiSessionResp(THolder<TResponse::TImpl>(
            new TResponse::TImpl(bbResponse)));

        return THolder<TMultiSessionResp>(resp);
    }

    // method = oauth
    TString OAuthRequest(const TStringBuf token, const TStringBuf userIp, const TOptions& options) {
        TRequestBuilder req(strMethodOAuth);
        req.Add("oauth_token", token);
        req.Add("userip", userIp);
        req.Add(options);

        return req;
        ;
    }

    TLoginReqData OAuthRequestSecure(const TStringBuf token, const TStringBuf userIp, const TOptions& options) {
        TRequestBuilder req(strMethodOAuth);
        req.Add("userip", userIp);
        req.Add(options);

        TLoginReqData loginData;
        loginData.Uri_ = req;
        loginData.PostData_.append("oauth_token=").append(URLEncode(token));

        return loginData;
    }
}
