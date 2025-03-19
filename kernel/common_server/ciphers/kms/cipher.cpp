#include "cipher.h"

#include <kernel/common_server/abstract/frontend.h>

#include <contrib/libs/jwt-cpp/include/jwt-cpp/jwt.h>
#include <chrono>

namespace {
    TString ToPemStr(const TString& str) {
        TVector<TString> list = StringSplitter(str).SplitByString("-----").ToList<TString>();
        if (list.size() != 5)
            return str;

        list[2] = JoinSeq("\n", StringSplitter(list[2]).SplitByString(" ").ToList<TString>());
        return JoinSeq("-----", list);
    }
}

namespace NCS {

    IAbstractCipher::TPtr TKmsCipherConfig::DoConstruct(const IBaseServer* server) const {
        return MakeAtomicShared<TKmsCipher>(*server, *this);
    }

    void TKmsCipherConfig::DoInit(const TYandexConfig::Section* section) {
        TBase::DoInit(section);
        KmsKeyId = section->GetDirectives().Value("KmsKeyId", KmsKeyId);
        PrivateKey = section->GetDirectives().Value("PrivateKey", PrivateKey);
        PublicKey = section->GetDirectives().Value("PublicKey", PublicKey);
        ServiceAccId = section->GetDirectives().Value("ServiceAccId", ServiceAccId);
        ServiceAccKeyId = section->GetDirectives().Value("ServiceAccKeyId", ServiceAccKeyId);
        AuthApiName = section->GetDirectives().Value("AuthApiName", AuthApiName);
        KeyApiName = section->GetDirectives().Value("KeyApiName", KeyApiName);
    }

    void TKmsCipherConfig::DoToString(IOutputStream& os) const {
        TBase::DoToString(os);
        os << "KmsKeyId: " << KmsKeyId << Endl;
        os << "PrivateKey: " << PrivateKey << Endl;
        os << "PublicKey: " << PublicKey << Endl;
        os << "ServiceAccId: " << ServiceAccId << Endl;
        os << "ServiceAccKeyId: " << ServiceAccKeyId << Endl;
        os << "AuthApiName: " << AuthApiName << Endl;
        os << "KeyApiName: " << KeyApiName << Endl;
    }


    class TKmsAuthRequest: public NExternalAPI::IHttpRequestWithJsonReport {
    private:
        CSA_DEFAULT(TKmsAuthRequest, TString, Token);

    public:
        virtual bool BuildHttpRequest(NNeh::THttpRequest& request) const override {
            request.SetUri("/iam/v1/tokens");
            NJson::TJsonValue postData;
            TJsonProcessor::Write(postData, "jwt", Token);
            request.SetPostData(postData);
            return true;
        }

        class TResponse: public TJsonResponse {
        private:
            CSA_DEFAULT(TResponse, TString, BearerToken);

        protected:
            virtual bool DoParseJsonReply(const NJson::TJsonValue& json) override {
                return TJsonProcessor::Read(json, "iamToken", BearerToken);
            }
        };
        TKmsAuthRequest(const TString& token)
            : Token(token)
        {
        }
    };

    void TKmsCipher::TAuthTokenRefreshAgent::Process(void* /*threadSpecificResource*/) {
        while (true) {
            if (Owner->ExpiredDeadline < TInstant::Now() - TDuration::Hours(1)) {
                Owner->TryUpdateToken(1);
            }
            Sleep(TDuration::Seconds(10));
        }
    }

    void TKmsCipher::TPrimaryVersionRefreshAgent::Process(void* /*threadSpecificResource*/) {
        while (true) {
            Owner->UpdatePrimaryVersion();
            Sleep(TDuration::Minutes(5));
        }
    }

    TString TKmsCipher::RequestAuthToken() const {
        std::string priv_key(ToPemStr(Config.GetPrivateKey()));
        std::string pub_key(ToPemStr(Config.GetPublicKey()));

        auto now = std::chrono::system_clock::now();
        auto expires_at = now + std::chrono::hours(1);
        auto serviceAccountId = Config.GetServiceAccId();
        auto keyId = Config.GetServiceAccKeyId();
        std::set<std::string> audience;
        audience.insert("https://iam.api.cloud.yandex.net/iam/v1/tokens");
        auto algorithm = jwt::algorithm::ps256(pub_key, priv_key);

        auto encoded_token = jwt::create()
                                 .set_key_id(keyId)
                                 .set_issuer(serviceAccountId)
                                 .set_audience(audience)
                                 .set_issued_at(now)
                                 .set_expires_at(expires_at)
                                 .sign(algorithm);

        const auto api = Server.GetSenderPtr(Config.GetAuthApiName());
        if (!api) {
            TFLEventLog::Error("failed update auth token: api not configured")("api", Config.GetAuthApiName());
            return "";
        }
        auto r = api->SendRequest<TKmsAuthRequest>(encoded_token);
        if (!r.IsSuccess()) {
            TFLEventLog::Error("failed update auth token: cannot get bearer token");
            return "";
        }
        return r.GetBearerToken();
    }

    TKmsCipher::TKmsCipher(const IBaseServer& server, const TKmsCipherConfig& config)
        : TBase(server, config)
    {
        TryUpdateToken(5);
        AuthTokenRefreshPool.Start(1);
        AuthTokenRefreshPool.SafeAddAndOwn(MakeHolder<TAuthTokenRefreshAgent>(this));

        PrimaryVersionRefreshPool.Start(1);
        PrimaryVersionRefreshPool.SafeAddAndOwn(MakeHolder<TPrimaryVersionRefreshAgent>(this));
    }

    TKmsCipher::~TKmsCipher() {
        AuthTokenRefreshPool.Stop();
        PrimaryVersionRefreshPool.Stop();
    }

    void TKmsCipher::TryUpdateToken(int num) const {
        for (auto i = 0; i < num; ++i) {
            auto token = RequestAuthToken();
            if (!!token) {
                TWriteGuard wg(AuthTokenMutex);
                BearerToken = token;
                ExpiredDeadline = TInstant::Now() + TDuration::Hours(1);
                break;
            }
        }
    }

    void TKmsCipher::UpdatePrimaryVersion() const {
        const auto api = Server.GetSenderPtr(Config.GetKeyApiName());
        if (!api) {
            TFLEventLog::Error("failed update primary version: api not configured");
            return;
        }

        const auto r = api->SendRequest<TGetRequest>(GetBearerToken(), Config.GetKmsKeyId());
        if (!r.IsSuccess()) {
            TFLEventLog::Error("failed update primary version: cannot get actual version");
            return;
        }

        const TString& newPrimaryVersion = r.GetPrimaryVersionId();
        if (GetPrimaryVersion() != newPrimaryVersion) {
            TWriteGuard wg(PrimaryVersionMutex);
            PrimaryVersion = newPrimaryVersion;
        }
    }

    bool TKmsEncodeRequest::BuildHttpRequest(NNeh::THttpRequest& request) const {
        request.AddHeader("Authorization", "Bearer " + Token);
        request.SetUri("/kms/v1/keys/" + KeyId + ":encrypt");
        NJson::TJsonValue postData;
        TJsonProcessor::Write(postData, "plaintext", Base64Encode(Plain));
        request.SetPostData(postData);
        return true;
    }

    bool TKmsDecodeRequest::BuildHttpRequest(NNeh::THttpRequest& request) const {
        request.AddHeader("Authorization", "Bearer " + Token);
        request.SetUri("/kms/v1/keys/" + KeyId + ":decrypt");
        NJson::TJsonValue postData;
        TJsonProcessor::Write(postData, "ciphertext", Base64Encode(Encoded));
        request.SetPostData(postData);
        return true;
    }

    bool TKmsCipher::TReEncryptRequest::BuildHttpRequest(NNeh::THttpRequest& request) const {
        request.AddHeader("Authorization", "Bearer " + Token);
        request.SetUri("/kms/v1/keys/" + KeyId + ":reEncrypt");
        NJson::TJsonValue postData;
        TJsonProcessor::Write(postData, "ciphertext", Base64Encode(Encrypted));
        TJsonProcessor::Write(postData, "sourceKeyId", KeyId);
        request.SetPostData(postData);
        return true;
    }

    bool TKmsCipher::TGetRequest::BuildHttpRequest(NNeh::THttpRequest& request) const {
        request.AddHeader("Authorization", "Bearer " + Token);
        request.SetUri("/kms/v1/keys/" + KeyId);
        request.SetRequestType("GET");
        return true;
    }

    bool TKmsEncodeRequest::TResponse::DoParseJsonReply(const NJson::TJsonValue& json) {
        TString base64Encoded;
        if (!TJsonProcessor::Read(json, "ciphertext", base64Encoded)) {
            return false;
        }
        Encoded = Base64DecodeUneven(base64Encoded);
        return true;
    }

    bool TKmsDecodeRequest::TResponse::DoParseJsonReply(const NJson::TJsonValue& json) {
        TString base64Encoded;
        if (!TJsonProcessor::Read(json, "plaintext", base64Encoded)) {
            return false;
        }
        Plain = Base64DecodeUneven(base64Encoded);
        if (!TJsonProcessor::Read(json, "versionId", VersionId)) {
            return false;
        }
        return true;
    }

    bool TKmsCipher::TReEncryptRequest::TResponse::DoParseJsonReply(const NJson::TJsonValue& json) {
        TString base64Encoded;
        if (!TJsonProcessor::Read(json, "ciphertext", base64Encoded)) {
            return false;
        }
        Encrypted = Base64DecodeUneven(base64Encoded);
        return true;
    }

    bool TKmsCipher::TGetRequest::TResponse::DoParseJsonReply(const NJson::TJsonValue& json) {
        TString base64Encoded;
        if (!json.Has("primaryVersion")) {
            return false;
        }
        if (!TJsonProcessor::Read(json["primaryVersion"], "id", PrimaryVersionId)) {
            return false;
        }
        TString status;
        if (!TJsonProcessor::Read(json["primaryVersion"], "status", status)) {
            return false;
        }
        return status == "ACTIVE";
    }

    bool TKmsCipher::ReencryptIfNeeded(TString& encrypted, bool& isReencrypted) const {
        isReencrypted = false;
        const auto api = Server.GetSenderPtr(Config.GetApiName());
        if (!api) {
            TFLEventLog::Error("Api not configured")("api", Config.GetAuthApiName());
            return false;
        }
        const auto r = api->SendRequest<TKmsDecodeRequest>(GetBearerToken(), Config.GetKmsKeyId(), encrypted);
        if (!r.IsSuccess()) {
            TFLEventLog::Error("cannot decrypt");
            return false;
        }
        if (GetPrimaryVersion() != r.GetVersionId()) {
            const auto r = api->SendRequest<TReEncryptRequest>(GetBearerToken(), Config.GetKmsKeyId(), encrypted);
            if (!r.IsSuccess()) {
                TFLEventLog::Error("cannot encrypt");
                return false;
            }
            encrypted = r.GetEncrypted();
            isReencrypted = true;
        }
        return true;
    }

    TKmsCipherConfig::TFactory::TRegistrator<TKmsCipherConfig> TKmsCipherConfig::Registrator("kms");
}
