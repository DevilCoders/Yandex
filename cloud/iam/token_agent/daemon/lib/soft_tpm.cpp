#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

#include <library/cpp/logger/global/global.h>
#include <util/generic/yexception.h>

#include "config.h"
#include "soft_tpm.h"
#include "rsa_key_pair.h"

namespace NTokenAgent {
    constexpr int RSA_BITS{2048};

    TSoftTpmAgentImpl::TSoftTpmAgentImpl(const TConfig& config)
        : KeyPath(config.GetKeyPath())
    {
        std::filesystem::create_directories(KeyPath);
        std::filesystem::permissions(KeyPath, std::filesystem::perms::owner_all);
    }

    grpc::Status TSoftTpmAgentImpl::Create(
        grpc::ServerContext* context,
        const NTpmAgent::CreateRequest* request,
        NTpmAgent::CreateResponse* reply) {
        Y_UNUSED(context);

        auto passwordLength = request->password().size();
        if (passwordLength <= 0 || passwordLength > 32) {
            WARNING_LOG << "Bad password length: " << passwordLength << "\n";
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                                "password: length must be in the range of 1 to 32");
        }

        if (request->hierarchy() != NTpmAgent::Hierarchy::OWNER) {
            WARNING_LOG << "Bad hierarchy: " << (int)request->hierarchy() << "\n";
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                                "hierarchy: Field is required");
        }

        ui64 handle;
        std::filesystem::path path;
        // Generate unique handle
        do {
            handle = std::rand();
            path = KeyPath / std::to_string(handle);
        } while (std::filesystem::exists(path));

        try {
            auto key_pair = TRsaKeyPair::Create(RSA_BITS);
            key_pair.Write(path, request->password());
            reply->set_pub(key_pair.GetPublicKey().c_str());
            reply->set_handle(long(handle));
        } catch (const std::exception& ex) {
            return grpc::Status(grpc::StatusCode::INTERNAL, ex.what());
        }

        return grpc::Status::OK;
    }

    grpc::Status TSoftTpmAgentImpl::ReadPublic(
        grpc::ServerContext* context,
        const NTpmAgent::ReadPublicRequest* request,
        NTpmAgent::ReadPublicResponse* reply) {
        Y_UNUSED(context);

        auto path = KeyPath / std::to_string(request->handle()).append(".pub");
        if (!std::filesystem::exists(path)) {
            return grpc::Status(grpc::StatusCode::NOT_FOUND,
                                "handle: not found");
        }

        try {
            auto rsa = ReadPublicKeyFromFile(path);
            auto public_key = GetKey(rsa.get(), PEM_write_bio_RSA_PUBKEY);
            reply->set_pub(public_key.c_str());
        } catch (const ssl_exception& ex) {
            if (ex.reason() == EVP_R_BAD_DECRYPT) {
                return grpc::Status(grpc::StatusCode::PERMISSION_DENIED,
                                    "password: not accepted");
            }
            return grpc::Status(grpc::StatusCode::INTERNAL, ex.what());
        } catch (const std::exception& ex) {
            return grpc::Status(grpc::StatusCode::INTERNAL, ex.what());
        }

        return grpc::Status::OK;
    }

    grpc::Status TSoftTpmAgentImpl::Sign(
        grpc::ServerContext* context,
        const NTpmAgent::SignRequest* request,
        NTpmAgent::SignResponse* reply) {
        Y_UNUSED(context);

        auto passwordLength = request->password().size();
        if (passwordLength <= 0 || passwordLength > 32) {
            WARNING_LOG << "Bad password length: " << passwordLength << "\n";
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                                "password: length must be in the range of 1 to 32");
        }
        if (request->scheme().alg() != NTpmAgent::Alg::RSASSA && request->scheme().alg() != NTpmAgent::Alg::RSAPSS) {
            WARNING_LOG << "Bad algorithm: " << int(request->scheme().alg()) << "\n";
            return grpc::Status(grpc::StatusCode::OUT_OF_RANGE,
                                "scheme: only RSASSA/RSAPSS algorithm supported");
        }
        if (request->scheme().hash() != NTpmAgent::Hash::SHA256 || request->digest().size() != 32) {
            WARNING_LOG << "Bad hash: " << int(request->scheme().hash()) << "\n";
            return grpc::Status(grpc::StatusCode::OUT_OF_RANGE,
                                "scheme: only SHA256 hash supported");
        }

        auto path = KeyPath / std::to_string(request->handle());
        if (!std::filesystem::exists(path)) {
            return grpc::Status(grpc::StatusCode::NOT_FOUND,
                                "handle: not found");
        }

        try {
            auto private_key = ReadPrivateKeyFromFile(path, request->password());
            auto signature = request->scheme().alg() == NTpmAgent::Alg::RSASSA
                                 ? SignatureRs256(private_key, request->digest())
                                 : SignaturePs256(private_key, request->digest());
            reply->set_signature(signature.c_str(), signature.size());
        } catch (const ssl_exception& ex) {
            if (ex.reason() == EVP_R_BAD_DECRYPT) {
                return grpc::Status(grpc::StatusCode::PERMISSION_DENIED,
                                    "password: not accepted");
            }
            return grpc::Status(grpc::StatusCode::INTERNAL, ex.what());
        } catch (const std::exception& ex) {
            return grpc::Status(grpc::StatusCode::INTERNAL, ex.what());
        }

        return grpc::Status::OK;
    }

}
