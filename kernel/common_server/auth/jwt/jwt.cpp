#include "jwt.h"

#include <kernel/common_server/library/openssl/rsa.h>
#include <kernel/common_server/util/instant_model.h>
#include <util/string/builder.h>

namespace {
    class TFieldLogsAccumulator: public NCS::NLogging::TDefaultLogsAccumulator {
    private:
        using TBase = NCS::NLogging::TDefaultLogsAccumulator;
        CSA_READONLY(TString, Field, "text")
        TVector<TString> FieldsLog;

    protected:
        virtual void DoAddRecord(const NCS::NLogging::TBaseLogRecord& r) override {
            TBase::DoAddRecord(r);
            for (const auto& i : r.GetFields()) {
                if (i.GetName() == Field) {
                    FieldsLog.emplace_back(i.GetValue());
                }
            }
        }

    public:
        using TBase::TBase;

        TFieldLogsAccumulator(const TString& field)
            : Field(field)
        {
        }
        virtual bool IsLogsAccumulatorEnabled() const override {
            return true;
        }

        virtual TString GetClassName() const override {
            return "fields-log";
        }

        TString GetAccumulatedFields() const {
            TStringBuilder sb;
            for (const auto& i : FieldsLog) {
                sb << " " << i << '.';
            }
            return sb;
        }
    };
}

IAuthInfo::TPtr TJwtAuthModule::DoRestoreAuthInfo(IReplyContext::TPtr requestContext) const {
    const auto& rd = requestContext->GetBaseRequestData();
    const auto& headers = rd.HeadersIn();
    const auto p = headers.find(Config.GetHeaderField());
    if (p == headers.end()) {
        return MakeAtomicShared<TJwtAuthInfo>("header " + Config.GetHeaderField() + " is missing");
    }

    const TVector<TString> items = StringSplitter(p->second).SplitByString(".").ToList<TString>();
    if (items.size() != 3) {
        return MakeAtomicShared<TJwtAuthInfo>("incorect jwt token: expect 3 items but actual is " + ToString(items.size()));
    }

    NJson::TJsonValue jwtHeader;
    NJson::ReadJsonFastTree(Base64DecodeUneven(items[0]), &jwtHeader);
    NJson::TJsonValue jwtPayload;
    NJson::ReadJsonFastTree(Base64DecodeUneven(items[1]), &jwtPayload);
    const TString jwtSignature = Base64DecodeUneven(items[2]);

    TString alg;
    if (jwtHeader.Has("alg") && jwtHeader["alg"].IsString()) {
        alg = jwtHeader["alg"].GetString();
    } else {
        return MakeAtomicShared<TJwtAuthInfo>("incorect jwt header: alg not found");
    }

    TString kid;
    if (jwtHeader.Has("kid") && jwtHeader["kid"].IsString()) {
        kid = jwtHeader["kid"].GetString();
    } else {
        return MakeAtomicShared<TJwtAuthInfo>("incorect jwt header: kid not found");
    }

    {
        THolder<TFieldLogsAccumulator> textAccumulator = MakeHolder<TFieldLogsAccumulator>("text");
        TFLEventLog::TContextGuard logText(textAccumulator.Get());
        if (!Verify(Join('.', items[0], items[1]), jwtSignature, kid, alg)) {
            return MakeAtomicShared<TJwtAuthInfo>("verification failed:" + textAccumulator->GetAccumulatedFields());
        }
    }

    if (jwtPayload.Has("exp") && jwtPayload["exp"].IsUInteger() && ModelingNow().Seconds() > jwtPayload["exp"].GetUInteger()) {
        return MakeAtomicShared<TJwtAuthInfo>("token is expired");
    }
    if (jwtPayload.Has("nbf") && jwtPayload["nbf"].IsUInteger() && ModelingNow().Seconds() < jwtPayload["nbf"].GetUInteger()) {
        return MakeAtomicShared<TJwtAuthInfo>("token is not available yet");
    }

    TString userId;
    if (jwtPayload.Has("sub") && jwtPayload["sub"].IsString()) {
        userId = jwtPayload["sub"].GetString();
    } else {
        userId = Config.GetDefaultUserId();
    }

    if (!!Config.GetSkipDomain()) {
        const auto pos = userId.find("\\");
        if (pos != TString::npos) {
            userId = userId.substr(pos + 1);
        }
    }

    size_t start = 0;
    size_t len = userId.size();
    if (!!Config.GetSkipPrefix() && userId.StartsWith(Config.GetSkipPrefix())) {
        start = Config.GetSkipPrefix().size();
        len -= start;
    }
    if (!!Config.GetSkipSuffix() && userId.EndsWith(Config.GetSkipSuffix())) {
        len -= Config.GetSkipSuffix().size();
    }
    userId = userId.substr(start, len);

    TString serviceId;
    if (jwtPayload.Has("iss") && jwtPayload["iss"].IsString()) {
        serviceId = jwtPayload["iss"].GetString();
    }

    return MakeAtomicShared<TJwtAuthInfo>(true, userId, serviceId);
}

bool TJwtAuthModule::Verify(const TString& message, const TString& signature, const TString& kid, const TString& algorithm) const {
    if (algorithm == "none") {
        if (!Config.GetAllowNoneAlg()) {
            TFLEventLog::Error("Not allowed none algorithm");
            return false;
        } else {
            return true;
        }
    } else if (algorithm == "RS256") {
        return VerifyRSA(message, signature, kid);
    }
    TFLEventLog::Error("Unsupported algorithm")("algorithm", algorithm);
    return false;
}

bool TJwtAuthModule::VerifyRSA(const TString& message, const TString& signature, const TString& kid) const {
    const auto jwks = GetJwkTokens();
    if (jwks.empty()) {
        TFLEventLog::Error("Failed get public keys");
        return false;
    }

    for (const auto& i : jwks) {
        if (i.GetKId() == kid) {
            NOpenssl::TRSAPublicKey key;
            if (!key.Init(i)) {
                TFLEventLog::Error("Failed read public key");
                return false;
            }
            if (!key.VerifySHA(message, signature)) {
                TFLEventLog::Error("Bad signature");
                return false;
            }
            return true;
        }
    }

    TFLEventLog::Error("Not found public key")("kid", kid);
    return false;
}

namespace {
    class TJwkRequest: public NExternalAPI::IHttpRequestWithJsonReport {
    private:
        CSA_READONLY_DEF(TString, Uri);

    public:
        TJwkRequest(const TString& uri)
            : Uri(uri)
        {
        }

        virtual bool BuildHttpRequest(NNeh::THttpRequest& request) const override {
            request.SetRequestType("GET");
            request.SetUri(Uri);
            return true;
        }

        class TResponse: public TJsonResponse {
            CSA_DEFAULT(TResponse, TVector<TJwk>, JwkList);

        protected:
            virtual bool DoParseJsonReply(const NJson::TJsonValue& json) override {
                if (!json.IsArray()) {
                    return false;
                }
                for (const auto& i : json.GetArray()) {
                    TJwk jwk;
                    if (jwk.DeserializeFromJson(i)) {
                        JwkList.emplace_back(std::move(jwk));
                    }
                }
                return !JwkList.empty();
            }
        };
    };
}

TVector<TJwk> TJwtAuthModule::GetJwkTokens() const {
    if (!JwkApi) {
        return {};
    }

    const auto r = JwkApi->SendRequest<TJwkRequest>(Config.GetJwksUri());
    if (!r.IsSuccess()) {
        return {};
    }

    return r.GetJwkList();
}

void TJwtAuthConfig::DoInit(const TYandexConfig::Section* section) {
    CHECK_WITH_LOG(section);
    const auto& directives = section->GetDirectives();
    HeaderField = directives.Value("HeaderField", HeaderField);
    DefaultUserId = directives.Value("DefaultUserId", DefaultUserId);
    SkipSuffix = directives.Value("SkipSuffix", SkipSuffix);
    SkipPrefix = directives.Value("SkipPrefix", SkipPrefix);
    SkipDomain = directives.Value("SkipDomain", SkipDomain);
    JwksApi = directives.Value("JwksApi", JwksApi);
    JwksUri = directives.Value("JwksUri", JwksUri);
}

void TJwtAuthConfig::DoToString(IOutputStream& os) const {
    os << "HeaderField: " << HeaderField << Endl;
    os << "DefaultUserId: " << DefaultUserId << Endl;
    os << "SkipSuffix: " << SkipSuffix << Endl;
    os << "SkipPrefix: " << SkipPrefix << Endl;
    os << "SkipDomain: " << SkipDomain << Endl;
    os << "JwksApi: " << JwksApi << Endl;
    os << "JwksUri: " << JwksUri << Endl;
}

THolder<IAuthModule> TJwtAuthConfig::DoConstructAuthModule(const IBaseServer* server) const {
    return MakeHolder<TJwtAuthModule>(*this, server->GetSenderPtr(JwksApi));
}

TJwtAuthConfig::TFactory::TRegistrator<TJwtAuthConfig> TJwtAuthConfig::Registrator("jwt");
