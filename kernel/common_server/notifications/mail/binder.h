#pragma once

#pragma once

#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/library/async_impl/async_impl.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/tvmauth/client/facade.h>
#include <library/cpp/logger/global/global.h>
#include <util/stream/file.h>
#include <library/cpp/string_utils/url/url.h>
#include <kernel/daemon/config/daemon_config.h>

class TAccountEmailBinderConfig {
    RTLINE_READONLY_ACCEPTOR(Host, TString, "passport-internal.yandex.ru");
    RTLINE_READONLY_ACCEPTOR(Port, ui32, 443);

    RTLINE_READONLY_ACCEPTOR(Consumer, TString, "y.drive-sessions");
    RTLINE_READONLY_ACCEPTOR(Language, TString, "ru");
    RTLINE_READONLY_ACCEPTOR(ValidatorUiUrl, TString, "https://passport.yandex.ru");
    RTLINE_READONLY_ACCEPTOR(RetPath, TString, "https://passport.yandex.ru");
    RTLINE_READONLY_ACCEPTOR(SubmitUri, TString, "/1/bundle/email/send_confirmation_email/");
    RTLINE_READONLY_ACCEPTOR(CommitUri, TString, "/1/bundle/email/confirm/by_code/");

    RTLINE_READONLY_ACCEPTOR(SelfTvmId, ui32, 0);
    RTLINE_READONLY_ACCEPTOR(DestinationTvmId, ui32, 0);

    RTLINE_READONLY_ACCEPTOR_DEF(RequestConfig, NSimpleMeta::TConfig);
    RTLINE_READONLY_ACCEPTOR(RequestTimeout, TDuration, TDuration::Seconds(1));

public:
    void Init(const TYandexConfig::Section* section);
    void ToString(IOutputStream& os) const;

    static TAccountEmailBinderConfig ParseFromString(const TString& configStr);
};
class TAccountEmailBinder {
private:
    TAccountEmailBinderConfig Config;
    TAtomicSharedPtr<TAsyncDelivery> AD;
    THolder<NNeh::THttpClient> Agent;
    TAtomicSharedPtr<NTvmAuth::TTvmClient> Tvm;

public:
    TAccountEmailBinder(const TAccountEmailBinderConfig& config, const TAtomicSharedPtr<NTvmAuth::TTvmClient>& tvmClient)
        : Config(config)
        , AD(MakeAtomicShared<TAsyncDelivery>())
        , Agent(MakeHolder<NNeh::THttpClient>(AD))
        , Tvm(tvmClient)
    {
        AD->Start(Config.GetRequestConfig().GetThreadsStatusChecker(), Config.GetRequestConfig().GetThreadsSenders());
        Agent->RegisterSource("mail-binder", Config.GetHost(), Config.GetPort(), Config.GetRequestConfig(), true);
    }
    ~TAccountEmailBinder() {
        AD->Stop();
    }
    bool BindSubmit(const TString& email, TString& token, const TString& userAuthHeader, const TString& clientIp, TString& error) const;
    bool ConfirmCode(const TString& key, const TString& token, const TString& userAuthHeader, const TString& clientIp, TString& error) const;

private:
    bool SendRequest(const TString& cgi, TString& token, const TString& uri, const TString& userAuthHeader, const TString& clientIp, TString& error) const;
    bool SendRequest(const TString& cgi, const TString& token, const TString& uri, const TString& userAuthHeader, const TString& clientIp, TString& error) const;
    bool CreateTrack(TString& trackId) const;
};
