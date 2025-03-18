#pragma once

#include "accessors.h"
#include "options.h"
#include "session_errors.h"

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NBlackbox2 {
    /// Default argument value in function signatures
    extern const TString EMPTY_STR;

    // ==== Response structure ============================================

    /// Basic response type
    class TResponse {
    public:
        class TImpl;

        TResponse(THolder<TImpl> pImpl);
        virtual ~TResponse();

        const TString& Message() const {
            return Message_;
        }

        TImpl* GetImpl() const {
            return Impl_.Get();
        } // backdoor for info accessors

    protected:
        THolder<TImpl> Impl_;
        TString Message_;
    };

    /// Bulk userinfo response
    class TBulkResponse: public TResponse {
    public:
        TBulkResponse(THolder<TImpl> pImpl);
        ~TBulkResponse();

        int Count() const {
            return Count_;
        }

        TString Id(int i) const;
        THolder<TResponse> User(int i) const;

    protected:
        int Count_;
    };

    // ==== Blackbox methods ==============================================

    /// User identification by login or login/sid
    struct TLoginSid {
        TString Login;
        TString Sid;

        explicit TLoginSid(const TString& login, const TString& sid = EMPTY_STR) // sid can be empty
            : Login(login)
            , Sid(sid)
        {
        }
    };

    /// User identification by suid/sid
    struct TSuidSid {
        TString Suid;
        TString Sid;

        TSuidSid(const TString& suid, const TString& sid)
            : Suid(suid)
            , Sid(sid)
        {
        }
    };

    // method = userinfo

    TString InfoRequest(
        const TStringBuf uid,
        const TStringBuf userIp,
        const TOptions& options = OPT_NONE);
    TString InfoRequest(
        const TLoginSid& loginsid,
        const TStringBuf userIp,
        const TOptions& options = OPT_NONE);
    TString InfoRequest(
        const TSuidSid& suidsid,
        const TStringBuf userIp,
        const TOptions& options = OPT_NONE);

    // Bulk userinfo request
    TString InfoRequest(
        const TVector<TString>& uids,
        const TStringBuf userIp,
        const TOptions& options = OPT_NONE);

    /// Parse given BB userinfo reply text and construct Response object
    THolder<TResponse> InfoResponse(const TStringBuf bbResponse);

    /// Parse given BB bulk userinfo reply text and construct BulkResponse object
    THolder<TBulkResponse> InfoResponseBulk(const TStringBuf bbResponse);

    // method = login

    /// Get request URI and POST data for login request
    struct TLoginReqData {
        TString Uri_;
        TString PostData_;
    };

    TLoginReqData LoginRequest(
        const TLoginSid& loginsid,
        const TStringBuf password,
        const TStringBuf authtype,
        const TStringBuf userIp,
        const TOptions& options = OPT_NONE);

    TLoginReqData LoginRequestUid(
        const TStringBuf uid,
        const TStringBuf password,
        const TStringBuf authtype,
        const TStringBuf userIp,
        const TOptions& options = OPT_NONE);

    /// Blackbox login method response
    class TLoginResp: public TResponse {
    public:
        // Login v1 status
        enum EStatus {
            Valid,
            Disabled,
            Invalid,
            ShowCaptcha,
            AlienDomain,
            Compromised,
            Expired
        };

        // Login v2 detailed status: Account+Password
        enum EAccountStatus {
            accUnknown,
            accValid,
            accAlienDomain,
            accNotFound,
            accDisabled
        };

        enum EPasswdStatus {
            pwdUnknown,
            pwdValid,
            pwdBad,
            pwdCompromised
        };

        TLoginResp(THolder<TImpl> pImpl);
        ~TLoginResp();

        EStatus Status() const {
            return Status_;
        }
        EAccountStatus AccStatus() const {
            return AccStatus_;
        }
        EPasswdStatus PwdStatus() const {
            return PwdStatus_;
        }

    private:
        EStatus Status_;
        EAccountStatus AccStatus_;
        EPasswdStatus PwdStatus_;
    };

    /// Parse given BB login reply text and construct LoginResp object
    THolder<TLoginResp> LoginResponse(const TStringBuf bbResponse);

    // method = sessionid

    TString SessionIDRequest(
        const TStringBuf sessionId,
        const TStringBuf hostname,
        const TStringBuf userIp,
        const TOptions& options = OPT_NONE);

    // method = oauth

    TString OAuthRequest(
        const TStringBuf token,
        const TStringBuf userIp,
        const TOptions& options = OPT_NONE);

    TLoginReqData OAuthRequestSecure(
        const TStringBuf token,
        const TStringBuf userIp,
        const TOptions& options = OPT_NONE);

    /// Blackbox sessionid method response
    class TSessionResp: public TResponse {
    public:
        enum EStatus {
            Valid,
            NeedReset,
            Expired,
            NoAuth,
            Disabled,
            Invalid,
            WrongGuard = 8,
        };
        TSessionResp(THolder<TImpl> pImpl);
        ~TSessionResp();

        EStatus Status() const {
            return Status_;
        }
        const TString& Age() const {
            return Age_;
        }
        bool IsLite() const {
            return IsLite_;
        }

    private:
        EStatus Status_;
        TString Age_;
        bool IsLite_;
    };

    /// Blackbox sessionid method response, multisession mode
    class TMultiSessionResp: public TResponse {
    public:
        TMultiSessionResp(THolder<TImpl> pImpl);
        ~TMultiSessionResp();

        TSessionResp::EStatus Status() const {
            return Status_;
        }
        const TString& Age() const {
            return Age_;
        }
        const TString& DefaultUid() const {
            return DefaultUid_;
        }

        int Count() const {
            return Count_;
        }

        TString Id(int i) const;
        THolder<TSessionResp> User(int i) const;

    private:
        TSessionResp::EStatus Status_;
        TString Age_;
        TString DefaultUid_;
        int Count_;
    };

    /// Parse given BB sessionid reply text and construct SessionResp object
    THolder<TSessionResp> SessionIDResponse(const TStringBuf bbResponse);
    THolder<TMultiSessionResp> SessionIDResponseMulti(const TStringBuf bbResponse);

    // ==== Exceptions ====================================================

    /// Generic Blackbox exception
    class TBBError: public std::runtime_error {
        NSessionCodes::ESessionError Code_;

    public:
        TBBError(NSessionCodes::ESessionError code, const TString& message)
            : std::runtime_error(message.c_str())
            , Code_(code)
        {
        }
        virtual ~TBBError() {
        }

        NSessionCodes::ESessionError Code() const {
            return Code_;
        }
    };

    /// Fatal errors: in case of such an error repeating request doesn't
    /// make sense (e.g. bad argument)
    class TFatalError: public TBBError {
    public:
        TFatalError(NSessionCodes::ESessionError code, const TString& message)
            : TBBError(code, message)
        {
        }
    };

    /// Temporary error: in case of such an error it makes sense to repeat
    /// request (e.g. network ot database timeout)
    class TTempError: public TBBError {
    public:
        TTempError(NSessionCodes::ESessionError code, const TString& message)
            : TBBError(code, message)
        {
        }
    };
}
