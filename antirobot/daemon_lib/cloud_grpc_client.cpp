#include "cloud_grpc_client.h"
#include "eventlog_err.h"

#include <util/string/printf.h>
#include <util/string/strip.h>
#include <util/system/env.h>

#include <library/cpp/grpc/common/time_point.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>


namespace NAntiRobot {

using NCloudApi::CaptchaSettingsService;
using NCloudApi::GetSettingsByClientKeyRequest;
using NCloudApi::GetSettingsByServerKeyRequest;
using NCloudApi::CaptchaSettings;

namespace {
    NThreading::TFuture<TErrorOr<TMaybeCaptchaSettings>> WrapOnError(const CaptchaSettings& settings, const grpc::Status& status) {
        TMaybeCaptchaSettings maybeSettings = Nothing();
        if (status.error_code() == grpc::StatusCode::NOT_FOUND) {
            return NThreading::MakeFuture(TErrorOr<TMaybeCaptchaSettings>(std::move(maybeSettings)));
        }
        if (!status.ok()) {
            return NThreading::MakeFuture(TErrorOr<TMaybeCaptchaSettings>(TError(__LOCATION__ + yexception()
                << "Get gRPC failed: "
                << status.error_message() << "; "
                << status.error_details()
            )));
        }
        maybeSettings = settings;
        return NThreading::MakeFuture(TErrorOr<TMaybeCaptchaSettings>(std::move(maybeSettings)));
    }
}


TCloudApiClient::TCloudApiClient() {
    grpc::ChannelArguments channelArgs;
    channelArgs.SetInt(GRPC_ARG_INITIAL_RECONNECT_BACKOFF_MS, 100);
    channelArgs.SetInt(GRPC_ARG_MAX_RECONNECT_BACKOFF_MS, 1000);
    channelArgs.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, ANTIROBOT_DAEMON_CONFIG.CloudCaptchaApiKeepAliveTime.MilliSeconds());
    channelArgs.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, ANTIROBOT_DAEMON_CONFIG.CloudCaptchaApiKeepAliveTimeout.MilliSeconds());
    Channel = grpc::CreateCustomChannel(ANTIROBOT_DAEMON_CONFIG.CloudCaptchaApiEndpoint, grpc::InsecureChannelCredentials(), channelArgs);
    Stub = CaptchaSettingsService::NewStub(Channel);
}

NThreading::TFuture<TErrorOr<TMaybeCaptchaSettings>> TCloudApiClient::GetSettingsByClientKey(const TString& siteKey) {
    grpc::ClientContext context;
    TInstant deadline = TInstant::Now() + ANTIROBOT_DAEMON_CONFIG.CloudCaptchaApiGetClientKeyTimeout;
    context.set_deadline(deadline);

    GetSettingsByClientKeyRequest keyReq;
    CaptchaSettings settings;
    keyReq.Setclient_key(siteKey);
    grpc::Status status = Stub->GetByClientKey(&context, keyReq, &settings);
    return WrapOnError(settings, status);
}

NThreading::TFuture<TErrorOr<TMaybeCaptchaSettings>> TCloudApiClient::GetSettingsByServerKey(const TString& siteKey) {
    grpc::ClientContext context;
    TInstant deadline = TInstant::Now() + ANTIROBOT_DAEMON_CONFIG.CloudCaptchaApiGetServerKeyTimeout;
    context.set_deadline(deadline);

    GetSettingsByServerKeyRequest keyReq;
    CaptchaSettings settings;
    keyReq.Setserver_key(siteKey);
    grpc::Status status = Stub->GetByServerKey(&context, keyReq, &settings);
    return WrapOnError(settings, status);
}

} // namespace NAntiRobot
