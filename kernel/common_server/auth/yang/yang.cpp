#include "yang.h"

#include <util/stream/file.h>


TYangAuthModule::TYangAuthModule(const TYangAuthConfig& config)
    : AuthToken(config.GetAuthToken())
    , VerifierUri(config.GetVerifierUri())
    , RequestsConfig(config.GetRequestsConfig())
    , CheckIsActivePair(config.GetCheckIsActivePair())
{
    if (CheckIsActivePair) {
        Requester = MakeHolder<NNeh::THttpClient>(config.GetAsyncDelivery());
        Requester->RegisterSource("yang", TString(config.GetHost()), config.GetPort(), RequestsConfig, config.GetHttps());
    }
}

IAuthInfo::TPtr TYangAuthModule::DoRestoreAuthInfo(IReplyContext::TPtr requestContext) const {
    if (!requestContext) {
        return nullptr;
    }

    TString assignmentId = "";
    TString secretId = "";
    if (!requestContext->GetCgiParameters().Has("assignmentId") || !requestContext->GetCgiParameters().Has("secretId")) {
        TBlob postData = requestContext->GetBuf();
        TMemoryInput inp(postData.Data(), postData.Size());
        NJson::TJsonValue jsonValue(NJson::JSON_NULL);
        if (!NJson::ReadJsonTree(&inp, &jsonValue)) {
            return nullptr;
        }
        if (!jsonValue.Has("assignmentId") || !jsonValue.Has("secretId") || !jsonValue["assignmentId"].IsString() || !jsonValue["secretId"].IsString()) {
            return nullptr;
        }
        assignmentId = jsonValue["assignmentId"].GetString();
        secretId = jsonValue["secretId"].GetString();
    } else {
        assignmentId = requestContext->GetCgiParameters().find("assignmentId")->second;
        secretId = requestContext->GetCgiParameters().find("secretId")->second;
    }

    if (assignmentId == "" || secretId == "") {
        return nullptr;
    }

    if (!CheckIsActivePair) {
        return MakeAtomicShared<TYangAuthInfo>(true, "yang-robot");
    }

    try {
        NNeh::THttpRequest request;
        TString requestUri = VerifierUri;
        if (!requestUri.EndsWith("/")) {
            requestUri += "/";
        }
        requestUri += assignmentId;
        request.SetUri(requestUri);
        request.AddHeader("Authorization", "OAuth " + AuthToken);
        auto result = Requester->SendMessageSync(request, Now() + TDuration::Seconds(1));

        if (!result.IsSuccessReply()) {
            ERROR_LOG << "unsuccessful reply for assignmentId=" << assignmentId << " secretId=" << secretId << Endl;
            return nullptr;
        }

        NJson::TJsonValue jsonContent;
        if (!NJson::ReadJsonFastTree(result.Content(), &jsonContent)) {
            ERROR_LOG << "non-json response for assignmentId=" << assignmentId << " secretId=" << secretId << Endl;
            return nullptr;
        }

        bool isActive = (jsonContent.Has("status") && jsonContent["status"].IsString() && jsonContent["status"].GetString() == "ACTIVE");
        if (!isActive) {
            ERROR_LOG << "non-active pair for assignmentId=" << assignmentId << " secretId=" << secretId << Endl;
            return nullptr;
        }

        bool isSecretIdValid = false;
        if (!jsonContent.Has("tasks") || !jsonContent["tasks"].IsArray()) {
            ERROR_LOG << "no 'tasks' array in response for assignmentId=" << assignmentId << " secretId=" << secretId << Endl;
            return nullptr;
        }
        for (auto&& entry : jsonContent["tasks"].GetArray()) {
            if (!entry.Has("input_values") || !entry["input_values"].IsMap()) {
                continue;
            }
            auto inputValues = entry["input_values"];
            if (inputValues.Has("secret") && inputValues["secret"].IsString()) {
                auto trueSecretId = inputValues["secret"].GetString();
                if (secretId == trueSecretId) {
                    isSecretIdValid = true;
                    break;
                }
            }
        }

        if (!isSecretIdValid) {
            ERROR_LOG << "invalid secretId for assignmentId=" << assignmentId << " secretId=" << secretId << Endl;
            return nullptr;
        }

        return MakeAtomicShared<TYangAuthInfo>(true, "yang-robot");
    } catch (...) {
        ERROR_LOG << "cannot check yang auth: " << CurrentExceptionMessage() << Endl;
        return nullptr;
    }
}

void TYangAuthConfig::DoInit(const TYandexConfig::Section* section) {
    if (!section) {
        WARNING_LOG << "nullptr YandexConfig section" << Endl;
        return;
    }

    const auto& directives = section->GetDirectives();
    {
        CheckIsActivePair = directives.Value("CheckIsActivePair", CheckIsActivePair);
        if (CheckIsActivePair) {
            TString tokenPath;
            tokenPath = directives.Value("TokenPath", tokenPath);
            AuthToken = Strip(TIFStream(tokenPath).ReadAll());
        }
        VerifierUri = directives.Value("VerifierUri", VerifierUri);
        Host = directives.Value("Host", Host);
        Port = directives.Value("Port", Port);
        Https = directives.Value("Https", Https);
    }

    AD = MakeAtomicShared<TAsyncDelivery>();
    AD->Start(RequestsConfig.GetThreadsStatusChecker(), RequestsConfig.GetThreadsSenders());
}

void TYangAuthConfig::DoToString(IOutputStream& os) const {
    os << "AuthToken: " << AuthToken << Endl;
    os << "VerifierUri: " << VerifierUri << Endl;
    os << "Host: " << Host << Endl;
    os << "Port: " << Port << Endl;
    os << "Https: " << Https << Endl;
    os << "CheckIsActivePair: " << CheckIsActivePair << Endl;

    os << "<RequestsConfig>" << Endl;
    RequestsConfig.ToString(os);
    os << "</RequestsConfig>" << Endl;
}

TYangAuthConfig::TFactory::TRegistrator<TYangAuthConfig> TYangAuthConfig::Registrator("yang");
