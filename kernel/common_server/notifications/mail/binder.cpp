#include "binder.h"
#include <kernel/common_server/library/logging/events.h>

void TAccountEmailBinderConfig::Init(const TYandexConfig::Section* section) {
    Host = section->GetDirectives().Value<TString>("Host", Host);
    Port = section->GetDirectives().Value<ui32>("Port", Port);

    Consumer = section->GetDirectives().Value<TString>("Consumer", Consumer);
    Language = section->GetDirectives().Value<TString>("Language", Language);
    ValidatorUiUrl = section->GetDirectives().Value<TString>("ValidatorUiUrl", ValidatorUiUrl);
    RetPath = section->GetDirectives().Value<TString>("RetPath", RetPath);
    SubmitUri = section->GetDirectives().Value<TString>("SubmitUri", SubmitUri);
    CommitUri = section->GetDirectives().Value<TString>("CommitUri", CommitUri);

    SelfTvmId = section->GetDirectives().Value<ui32>("SelfTvmId", SelfTvmId);
    DestinationTvmId = section->GetDirectives().Value<ui32>("DestinationTvmId", DestinationTvmId);

    const TYandexConfig::TSectionsMap children = section->GetAllChildren();
    {
        auto it = children.find("RequestConfig");
        if (it != children.end()) {
            RequestConfig.InitFromSection(it->second);
        }
    }
    RequestTimeout = section->GetDirectives().Value<TDuration>("RequestTimeout", RequestTimeout);
}

void TAccountEmailBinderConfig::ToString(IOutputStream& os) const {
    os << "Host: " << Host << Endl;
    os << "Port: " << Port << Endl;

    os << "Consumer: " << Consumer << Endl;
    os << "Language: " << Language << Endl;
    os << "ValidatorUiUrl: " << ValidatorUiUrl << Endl;
    os << "RetPath: " << RetPath << Endl;
    os << "SubmitUri: " << SubmitUri << Endl;
    os << "CommitUri: " << CommitUri << Endl;

    os << "SelfTvmId: " << SelfTvmId << Endl;
    os << "DestinationTvmId: " << DestinationTvmId << Endl;

    os << "<RequestConfig>" << Endl;
    RequestConfig.ToString(os);
    os << "</RequestConfig>" << Endl;
    os << "RequestTimeout: " << RequestTimeout << Endl;
}

TAccountEmailBinderConfig TAccountEmailBinderConfig::ParseFromString(const TString& configStr) {
    TAccountEmailBinderConfig result;
    TAnyYandexConfig config;
    CHECK_WITH_LOG(config.ParseMemory(configStr.data()));
    result.Init(config.GetRootSection());
    return result;
}

bool TAccountEmailBinder::CreateTrack(TString& trackId) const {
    NNeh::THttpRequest request;
    request
        .SetUri("/1/track/")
        .SetCgiData("consumer=" + Config.GetConsumer())
        .SetRequestType("POST")
        .AddHeader("X-Ya-Service-Ticket", Tvm->GetServiceTicketFor(Config.GetDestinationTvmId()))
        .AddHeader("Content-Type", "application/x-www-form-urlencoded");

    auto tgResult = Agent->SendMessageSync(request, Now() + Config.GetRequestTimeout());
    if (tgResult.Code() != HTTP_OK) {
        TFLEventLog::Log("Track creation failure",TLOG_ERR)
            ("request", request.GetDebugRequest())
            ("code", tgResult.Code())
            ("content", tgResult.Content())
            ("error message", tgResult.ErrorMessage());
        return false;
    }

    NJson::TJsonValue jsonResponse;
    if (!NJson::ReadJsonFastTree(tgResult.Content(), &jsonResponse)) {
        TFLEventLog::Log("Track creation failure", TLOG_ERR)
            ("reason", "response not json")
            ("request", request.GetDebugRequest())
            ("code", tgResult.Code())
            ("content", tgResult.Content());
        return false;
    }

    trackId = jsonResponse["id"].GetString();
    return true;
}

bool TAccountEmailBinder::SendRequest(const TString& requestCgi, TString& trackId, const TString& uri, const TString& userAuthHeader, const TString& clientIp, TString& error) const {
    if (!CreateTrack(trackId)) {
        return false;
    }
    return SendRequest(requestCgi, static_cast<const TString&>(trackId), uri, userAuthHeader, clientIp, error);
}

bool TAccountEmailBinder::SendRequest(const TString& requestCgi, const TString& trackId, const TString& uri, const TString& userAuthHeader, const TString& clientIp, TString& error) const {
    TString cgi = requestCgi;
    cgi += "&track_id=" + trackId;
    cgi += "&consumer=" + Config.GetConsumer();

    NNeh::THttpRequest request;
    request
        .SetUri("/" + uri)
        .SetCgiData("consumer=" + Config.GetConsumer())
        .SetPostData(cgi)
        .SetRequestType("POST")
        .AddHeader("X-Ya-Service-Ticket", Tvm->GetServiceTicketFor(Config.GetDestinationTvmId()))
        .AddHeader("Ya-Consumer-Authorization", userAuthHeader)
        .AddHeader("Ya-Consumer-Client-Ip", clientIp)
        .AddHeader("Content-Type", "application/x-www-form-urlencoded");

    auto tgResult = Agent->SendMessageSync(request, Now() + Config.GetRequestTimeout());
    if (tgResult.Code() != HTTP_OK) {
        TFLEventLog::Log("EMail bind failure", TLOG_ERR)
            ("request", request.GetDebugRequest())
            ("code", tgResult.Code())
            ("content", tgResult.Content())
            ("error message", tgResult.ErrorMessage());
        return false;
    }

    NJson::TJsonValue jsonResponse;
    if (!NJson::ReadJsonFastTree(tgResult.Content(), &jsonResponse)) {
        TFLEventLog::Log("EMail bind failure", TLOG_ERR)
            ("reason", "response not json")
            ("request", request.GetDebugRequest())
            ("code", tgResult.Code())
            ("content", tgResult.Content());
        return false;
    }

    if (!jsonResponse.Has("status") || !jsonResponse["status"].IsString()) {
        return false;
    }

    if (jsonResponse["status"].GetString() != "ok") {
        if (!jsonResponse["errors"].GetArray().empty()) {
            error = jsonResponse["errors"].GetArray()[0].GetStringRobust();
        }
        return false;
    }
    return true;
}

bool TAccountEmailBinder::BindSubmit(const TString& email, TString& token, const TString& userAuthHeader, const TString& clientIp, TString& error) const {
    TString cgi = "email=" + email;
    cgi += "&language=" + Config.GetLanguage();
    cgi += "&validator_ui_url=" + Config.GetValidatorUiUrl();
    cgi += "&retpath=" + Config.GetRetPath();
    cgi += "&is_safe=False";
    return SendRequest(cgi, token, Config.GetSubmitUri(), userAuthHeader, clientIp, error);
}

bool TAccountEmailBinder::ConfirmCode(const TString& key, const TString& token, const TString& userAuthHeader, const TString& clientIp, TString& error) const {
    TString cgi = "key=" + key;
    return SendRequest(cgi, token, Config.GetCommitUri(), userAuthHeader, clientIp, error);
}
