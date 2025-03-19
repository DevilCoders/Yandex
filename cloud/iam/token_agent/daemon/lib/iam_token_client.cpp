#include <sstream>

#include <library/cpp/logger/global/global.h>
#include <contrib/libs/jwt-cpp/include/jwt-cpp/jwt.h>

#include "iam_token_client.h"
#include "iam_token_client_utils.h"
#include "tpm_sign.h"
#include "mon.h"
#include "server.h"

namespace NTokenAgent {
    constexpr int MAX_CALL_DELAY_SECONDS{5};

    TIamTokenClient::TIamTokenClient(const TConfig& config, const std::shared_ptr<grpc::Channel>& IamTokenServiceChannel,
                                     const std::shared_ptr<grpc::Channel>& TpmAgentChannel)
        : JwtLifetime(config.GetJwtLifetime())
        , TsRequestTimeout(config.GetTokenServiceEndpoint().GetTimeout())
        , TsRetries(config.GetTokenServiceEndpoint().GetRetries())
        , TpmRequestTimeout(config.GetTpmAgentEndpoint().GetTimeout())
        , TpmRetries(config.GetTpmAgentEndpoint().GetRetries())
        , JwtAudience(config.GetJwtAudience())
        , IamTokenService(NProtoIam::IamTokenService::NewStub(IamTokenServiceChannel))
        , TpmAgent(NProtoTpmAgent::TpmAgent::NewStub(TpmAgentChannel))
    {}

    TIamTokenClient::TIamTokenClient(const TConfig& config)
        : TIamTokenClient(config,
              TIamTokenClientUtils::CreateChannel(config.GetTokenServiceEndpoint()),
              TIamTokenClientUtils::CreateChannel(config.GetTpmAgentEndpoint()))
    {}

    TToken TIamTokenClient::CreateIamToken(const TRole& role) {
        auto jwt = CreateJwt(role);
        DEBUG_LOG << "JWT: " << jwt << "\n";
        return CreateIamToken(jwt);
    }

    TToken TIamTokenClient::CreateIamToken(const std::string& jwt) {
        NProtoIam::CreateIamTokenRequest request;
        NProtoIam::CreateIamTokenResponse reply;
        request.set_jwt(jwt.c_str());

        for (int retry = 1; retry <= TsRetries; ++retry) {
            if (!TServer::IsRunning()) {
                break;
            }
            auto request_id = GenerateRequestId();
            grpc::ClientContext context;
            auto deadline = std::chrono::system_clock::now() + std::chrono::microseconds(TsRequestTimeout.MicroSeconds());
            context.set_deadline(deadline);
            context.AddMetadata("x-request-id", request_id);

            INFO_LOG << "Sending request to Token Service with request id "
                     << request_id.c_str()
                     << ", timeout " << TsRequestTimeout.ToString()
                     << ", try " << retry << " of " << TsRetries << "\n";
            auto status = IamTokenService->Create(&context, request, &reply);

            if (status.error_code() == grpc::StatusCode::UNAUTHENTICATED) {
                TMon::Get()->RevokedKey();
                ERROR_LOG << "Unable to create IAM token: request_id " << request_id.c_str()
                          << ", code " << ui32(status.error_code())
                          << ", message \"" << status.error_message() << "\"\n";
                return TToken("", TInstant::Now());
            } else if (status.error_code() == grpc::StatusCode::OK) {
                const auto& token = reply.iam_token();
                const auto& timestamp = reply.expires_at();
                const auto& expires_at = TInstant::MicroSeconds(
                    timestamp.seconds() * 1000000ULL + timestamp.nanos() / 1000ULL);

                return TToken(token, expires_at);
            }

            TMon::Get()->TokenServiceError();
            ERROR_LOG << "Failed to create IAM token: request_id " << request_id.c_str()
                      << ", code " << ui32(status.error_code())
                      << ", message \"" << status.error_message() << "\"\n";
            auto delay = std::chrono::seconds(std::min(retry, MAX_CALL_DELAY_SECONDS));
            std::this_thread::sleep_for(delay);
        }

        ythrow yexception() << "Failed to create IAM token";
    }

    std::string TIamTokenClient::CreateJwt(const TRole& role) {
        DEBUG_LOG << "SA id " << role.GetServiceAccountId()
                  << " key id " << role.GetKeyId() << "\n";
        auto password = TIamTokenClientUtils::ReadPassword();
        auto now = std::chrono::system_clock::now();
        auto expires_at = now + std::chrono::microseconds(JwtLifetime.MicroSeconds());
        auto audience = JwtAudience;
        TTpmSign tpm_sign(TpmAgent, role.GetKeyHandle(), password, TpmRetries, TpmRequestTimeout);

        try {
            return jwt::create()
                .set_key_id(role.GetKeyId())
                .set_issuer(role.GetServiceAccountId())
                .set_audience(audience)
                .set_issued_at(now)
                .set_expires_at(expires_at)
                .sign(tpm_sign);
        } catch (const std::runtime_error& ex) {
            ythrow yexception() << ex.what();
        }
    }
}
