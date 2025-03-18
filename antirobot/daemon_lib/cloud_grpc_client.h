#pragma once

#include "request_params.h"

#include <library/cpp/threading/future/future.h>

#include <antirobot/lib/error.h>
#include <yandex/cloud/priv/smartcaptcha/v1/captcha_service.grpc.pb.h>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>


namespace NAntiRobot {

namespace NCloudApi {
    using namespace yandex::cloud::priv::smartcaptcha::v1;
}

using TMaybeCaptchaSettings = TMaybe<NCloudApi::CaptchaSettings>;
class TCaptchaSettingsPtr : public TAtomicSharedPtr<NCloudApi::CaptchaSettings> {
public:
    using TBase = TAtomicSharedPtr<NCloudApi::CaptchaSettings>;

    explicit TCaptchaSettingsPtr(const TMaybeCaptchaSettings& settings)
        : TBase(new NCloudApi::CaptchaSettings) {
        (*this)->CopyFrom(*settings.Get());
    }

    TCaptchaSettingsPtr() = default;
    TCaptchaSettingsPtr(const TCaptchaSettingsPtr& ) = default;
    TCaptchaSettingsPtr(TCaptchaSettingsPtr&& ) = default;
    TCaptchaSettingsPtr(TBase&& p) : TBase(std::move(p)) {}

    static TCaptchaSettingsPtr CreateWithSiteKey(const TString& siteKey) {
        TCaptchaSettingsPtr res = MakeAtomicShared<NCloudApi::CaptchaSettings>();
        res->Setclient_key(siteKey);
        return res;
    }
};

class ICloudApiClient {
public:
    virtual NThreading::TFuture<TErrorOr<TMaybeCaptchaSettings>> GetSettingsByClientKey(const TString& siteKey) = 0;
    virtual NThreading::TFuture<TErrorOr<TMaybeCaptchaSettings>> GetSettingsByServerKey(const TString& siteKey) = 0;

    virtual ~ICloudApiClient() {
    }
};

class TDummyCloudApiClient : public ICloudApiClient {
    NThreading::TFuture<TErrorOr<TMaybeCaptchaSettings>> GetSettingsByClientKey(const TString&) override {
        TMaybeCaptchaSettings maybeSettings = Nothing();
        return NThreading::MakeFuture(TErrorOr<TMaybeCaptchaSettings>(std::move(maybeSettings)));
    }

    NThreading::TFuture<TErrorOr<TMaybeCaptchaSettings>> GetSettingsByServerKey(const TString&) override {
        TMaybeCaptchaSettings maybeSettings = Nothing();
        return NThreading::MakeFuture(TErrorOr<TMaybeCaptchaSettings>(std::move(maybeSettings)));
    }
};

class TCloudApiClient : public ICloudApiClient {

    std::shared_ptr<grpc::Channel> Channel;
    std::unique_ptr<NCloudApi::CaptchaSettingsService::Stub> Stub;

public:
    TCloudApiClient();

    NThreading::TFuture<TErrorOr<TMaybeCaptchaSettings>> GetSettingsByClientKey(const TString& siteKey) override;
    NThreading::TFuture<TErrorOr<TMaybeCaptchaSettings>> GetSettingsByServerKey(const TString& siteKey) override;
};

inline ICloudApiClient* CreateCloudApiClient() {
    if (ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService) {
        return new TCloudApiClient;
    }
    return new TDummyCloudApiClient;
}

} // namespace NAntiRobot
