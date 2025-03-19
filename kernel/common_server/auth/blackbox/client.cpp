#include "client.h"
#include <library/cpp/blackbox2/src/responseimpl.h> // WTH??
#include <library/cpp/blackbox2/src/xconfig.h> // WTH??
#include <util/string/split.h>
#include <util/string/join.h>

namespace NCS {

    template <class R>
    class TBlackboxClientRequest final: public NExternalAPI::IServiceApiHttpRequest {
    public:
        using TResponse = R;
        TBlackboxClientRequest(const TString& request)
            : Request(request)
        {}

        virtual bool BuildHttpRequest(NNeh::THttpRequest& request) const override {
            request.SetCgiData(Request);
            return true;
        }
    private:
        TString Request;
    };

    class TBlackboxClientResponse : public NExternalAPI::IServiceApiHttpRequest::IBaseResponse {
        CSA_READONLY_PROTECTED_DEF(TBlackboxClient::TResponsePtr, Response);
    };

    class TBBClientInfoResponse final: public TBlackboxClientResponse {
        virtual bool DoParseHttpResponse(const NUtil::THttpReply& reply) override {
            Response = NBlackbox2::InfoResponse(reply.Content());
            return true;
        }
    };

    class TBBClientSessionIDResponse final : public TBlackboxClientResponse {
        virtual bool DoParseHttpResponse(const NUtil::THttpReply& reply) override {
            Response = NBlackbox2::SessionIDResponse(reply.Content());
            return true;
        }
    };

    template<class TResponse>
    NThreading::TFuture<TBlackboxClient::TResponsePtr> TBlackboxClient::MakeRequest(const TString& request) const {
        return Sender->SendRequestAsync<TBlackboxClientRequest<TResponse>>(request).Apply(
            [](const NThreading::TFuture<TResponse>& r) -> TResponsePtr {
                return r.GetValue().GetResponse();
            }
        );
    }

    TBlackboxClient::TBlackboxClient(NExternalAPI::TSender::TPtr sender)
        : Sender(sender)
    {
        Options << NBlackbox2::OPT_GET_ALL_ALIASES;
        Options << NBlackbox2::OPT_GET_ALL_EMAILS;
    }

    NThreading::TFuture<TBlackboxClient::TResponsePtr> TBlackboxClient::LoginInfoRequest(TStringBuf login, TStringBuf userIp) const {
        TString l(login);
        NBlackbox2::TLoginSid loginSid(l);
        auto request = NBlackbox2::InfoRequest(loginSid, userIp, Options);
        return MakeRequest<TBBClientInfoResponse>(request);
    }

    NThreading::TFuture<TBlackboxClient::TResponsePtr> TBlackboxClient::UidInfoRequest(TStringBuf uid, TStringBuf userIp) const {
        auto request = NBlackbox2::InfoRequest(uid, userIp, Options);
        return MakeRequest<TBBClientInfoResponse>(request);
    }

    NThreading::TFuture<TBlackboxClient::TResponsePtr> TBlackboxClient::OAuthRequest(TStringBuf token, TStringBuf userIp) const {
        auto request = NBlackbox2::OAuthRequest(token, userIp, Options);
        return MakeRequest<TBBClientSessionIDResponse>(request);
    }

    NThreading::TFuture<TBlackboxClient::TResponsePtr> TBlackboxClient::SessionIdRequest(TStringBuf sessionId, TStringBuf userIp) const {
        auto request = NBlackbox2::SessionIDRequest(sessionId, CookieHost, userIp, Options);
        return MakeRequest<TBBClientSessionIDResponse>(request);
    }

    TBlackboxInfo TBlackboxClient::Parse(const NBlackbox2::TResponse& response) const {
        NBlackbox2::TUid userId(&response);
        NBlackbox2::TLoginInfo loginInfo(&response);
        NBlackbox2::TAttributes attrs(&response);
        NBlackbox2::TAliasList aliases(&response);
        NBlackbox2::TEmailList emails(&response);

        TBlackboxInfo info;
        info.Login = loginInfo.Login();
        info.PassportUid = userId.Uid();
        info.IsPlusUser = attrs.Get("1015") == "1";
        info.FirstName = attrs.Get("27");
        info.LastName = attrs.Get("28");
        info.BirthDate = attrs.Get("30");
        for (auto&& item : aliases.GetAliases()) {
            if (item.type() == NBlackbox2::TAliasList::TItem::EType::Yandexoid) {
                info.IsYandexoid = true;
                break;
            }
        }
        if (IsUserTicketsEnabled()) {
            NBlackbox2::TUserTicket userTicket(&response);
            info.TVMTicket = userTicket.Get();
        }

        if (auto responseImpl = response.GetImpl()) {
            auto defaultEmailItem = emails.GetDefaultItem();
            if (!!defaultEmailItem) {
                info.DefaultEmail = defaultEmailItem->Address();
            }

            for (const auto& email : emails.GetEmailItems()) {
                if (email.Validated()) {
                    info.ValidatedMails.push_back(email.Address());
                }
            }

            NBlackbox2::xmlConfig::Parts phones(responseImpl->GetParts("phones/phone"));
            if (phones.Size() == 1) {
                TString defaultPhoneStr;
                phones[0].GetIfExists("attribute", defaultPhoneStr);
                if (defaultPhoneStr.size() > 0) {
                    info.DefaultPhone = std::move(defaultPhoneStr);
                }
            }

            TString deviceId;
            if (responseImpl->GetIfExists("OAuth/device_id", deviceId)) {
                info.DeviceId = deviceId;
            }

            TString deviceName;
            if (responseImpl->GetIfExists("OAuth/device_name", deviceName)) {
                info.DeviceName = deviceName;
            }

            TString clientId;
            if (responseImpl->GetIfExists("OAuth/client_id", clientId)) {
                info.ClientId = clientId;
            }

            TString scope;
            if (responseImpl->GetIfExists("OAuth/scope", scope)) {
                StringSplitter(scope).Split(' ').Collect(&info.Scopes);
            }
        }
        return info;
    }

void NCS::TBlackboxClient::SetScopes(TConstArrayRef<TString> values) {
    Options << NBlackbox2::TOption("scopes", JoinSeq(",", values));
}

}
