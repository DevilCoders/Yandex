#include <library/cpp/logger/global/global.h>

#include "iam_token_client_utils.h"
#include "tpm_sign.h"

#include "mon.h"

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/pem.h>
#include <openssl/ec.h>
#include <openssl/err.h>
#include "ssl_util.h"

namespace NTokenAgent {
    static const grpc::string DEFAULT_PASSWORD("token-agent");
    constexpr int MAX_CALL_DELAY_SECONDS{5};

    TTpmSign::TTpmSign(std::shared_ptr<NProtoTpmAgent::TpmAgent::Stub> tpm_agent,
                       ui64 key_handle,
                       std::string key_password,
                       int retries,
                       TDuration request_timeout)
        : TpmAgent(std::move(tpm_agent))
        , KeyHandle(key_handle)
        , KeyPassword(std::move(key_password))
        , Retries(retries)
        , RequestTimeout(request_timeout)
    {
    }

    std::string TTpmSign::sign(const std::string& data) const {
        auto hash = Sha256Hash(data);

        NProtoTpmAgent::SignRequest request;
        NProtoTpmAgent::SignResponse reply;
        request.set_handle(long(KeyHandle));
        if (KeyPassword.empty()) {
            request.set_password(DEFAULT_PASSWORD.c_str(), DEFAULT_PASSWORD.size());
        } else {
            request.set_password(KeyPassword.c_str(), KeyPassword.size());
        }
        request.set_digest(hash.c_str(), hash.size());
        request.mutable_scheme()->set_alg(NProtoTpmAgent::Alg::RSASSA);
        request.mutable_scheme()->set_hash(NProtoTpmAgent::Hash::SHA256);

        for (int retry = 1; retry <= Retries; ++retry) {
            grpc::ClientContext context;
            auto request_id = GenerateRequestId();
            auto deadline = std::chrono::system_clock::now() + std::chrono::microseconds(RequestTimeout.MicroSeconds());
            context.set_deadline(deadline);
            context.AddMetadata("x-request-id", request_id);

            INFO_LOG << "Signing request with TPM Agent"
                     << ", timeout " << RequestTimeout.ToString()
                     << ", try " << retry << " of " << Retries << "\n";
            auto status = TpmAgent->Sign(&context, request, &reply);
            if (status.error_code() == grpc::StatusCode::PERMISSION_DENIED) {
                request.set_password(DEFAULT_PASSWORD.c_str(), DEFAULT_PASSWORD.size());
                continue;
            } else if (status.error_code() == grpc::StatusCode::OK) {
                return reply.signature();
            }

            TMon::Get()->TpmAgentError();
            DEBUG_LOG << "Failed to sign request with TPM Agent: code "
                      << (int)status.error_code()
                      << ", message \"" << status.error_message() << "\"\n";
            auto delay = std::chrono::seconds(std::min(retry, MAX_CALL_DELAY_SECONDS));
            std::this_thread::sleep_for(delay);
        }

        ythrow yexception() << "TPM failed";
    }
}
