#pragma once

#include <chrono>
#include <ostream>
#include <string>
#include <variant>

namespace yandex::cloud::auth {

    class IamToken {
    public:
        explicit IamToken(std::string value)
            : Value_(std::move(value))
        {
        }

        const std::string& Value() const {
            return Value_;
        }

    private:
        std::string Value_;
    };

    std::ostream& operator<<(std::ostream& out, const IamToken& token);

    class ApiKey {
    public:
        explicit ApiKey(std::string value)
            : Value_(std::move(value))
        {
        }

        const std::string& Value() const {
            return Value_;
        }

    private:
        std::string Value_;
    };

    std::ostream& operator<<(std::ostream& out, const ApiKey& key);

    enum SignatureMethod {
        SIGNATURE_METHOD_UNSPECIFIED,
        HMAC_SHA1,
        HMAC_SHA256,
    };

    class Version2Parameters {
    public:
        explicit Version2Parameters(SignatureMethod signature_method)
            : SignatureMethod_(signature_method)
        {
        }

        SignatureMethod GetSignatureMethod() const {
            return SignatureMethod_;
        }

    private:
        enum SignatureMethod SignatureMethod_;
    };

    std::ostream& operator<<(std::ostream& out, const Version2Parameters& parameters);

    class Version4Parameters {
    public:
        Version4Parameters(
            std::chrono::time_point<std::chrono::system_clock> signed_at,
            std::string region,
            std::string service)
            : SignedAt_(std::move(signed_at))
            , Region_(std::move(region))
            , Service_(std::move(service))
        {
        }

        const std::chrono::time_point<std::chrono::system_clock>& SignedAt() const {
            return SignedAt_;
        }

        const std::string& Region() const {
            return Region_;
        }

        const std::string& Service() const {
            return Service_;
        }

    private:
        std::chrono::time_point<std::chrono::system_clock> SignedAt_;
        std::string Region_;
        std::string Service_;
    };

    std::ostream& operator<<(std::ostream& out, const Version4Parameters& parameters);

    class AccessKeySignature {
    public:
        using AccessKeySignatureParameters = std::variant<Version2Parameters, Version4Parameters>;

        AccessKeySignature(
            std::string access_key_id,
            std::string signed_string,
            std::string signature,
            Version2Parameters parameters)
            : AccessKeyId_(std::move(access_key_id))
            , SignedString_(std::move(signed_string))
            , Signature_(std::move(signature))
            , Parameters_(std::move(parameters))
        {
        }

        AccessKeySignature(
            std::string access_key_id,
            std::string signed_string,
            std::string signature,
            Version4Parameters parameters)
            : AccessKeyId_(std::move(access_key_id))
            , SignedString_(std::move(signed_string))
            , Signature_(std::move(signature))
            , Parameters_(std::move(parameters))
        {
        }

        const std::string& AccessKeyId() const {
            return AccessKeyId_;
        }

        const std::string& SignedString() const {
            return SignedString_;
        }

        const std::string& Signature() const {
            return Signature_;
        }

        const AccessKeySignatureParameters& Parameters() const {
            return Parameters_;
        }

    private:
        std::string AccessKeyId_;
        std::string SignedString_;
        std::string Signature_;
        AccessKeySignatureParameters Parameters_;
    };

    std::ostream& operator<<(std::ostream& out, const AccessKeySignature& signature);

    using Credentials = std::variant<IamToken, ApiKey, AccessKeySignature>;

}
