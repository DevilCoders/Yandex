#pragma once

#include "info.h"

#include <kernel/common_server/library/tvm_services/abstract/abstract.h>
#include <kernel/common_server/util/accessor.h>
#include <library/cpp/blackbox2/blackbox2.h>

namespace NCS {

    class TBlackboxClient {
    public:
        using TResponsePtr = TAtomicSharedPtr<NBlackbox2::TResponse>;

    public:
        TBlackboxClient(NExternalAPI::TSender::TPtr sender);
        NThreading::TFuture<TResponsePtr> LoginInfoRequest(TStringBuf login, TStringBuf userIp) const;
        NThreading::TFuture<TResponsePtr> UidInfoRequest(TStringBuf uid, TStringBuf userIp) const;
        NThreading::TFuture<TResponsePtr> OAuthRequest(TStringBuf token, TStringBuf userIp) const;
        NThreading::TFuture<TResponsePtr> SessionIdRequest(TStringBuf sessionId, TStringBuf userIp) const;

        TBlackboxInfo Parse(const NBlackbox2::TResponse& response) const;
        void SetScopes(TConstArrayRef<TString> values);

        CSA_DEFAULT(TBlackboxClient, TString, CookieHost);
        CSA_FLAG(TBlackboxClient, UserTicketsEnabled, false);
        CSA_DEFAULT(TBlackboxClient, NBlackbox2::TOptions, Options);

    private:
        template<class TResponse>
        NThreading::TFuture<TResponsePtr> MakeRequest(const TString& request) const;
        NExternalAPI::TSender::TPtr Sender;
    };

}
