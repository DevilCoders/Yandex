#include "request_session.h"

namespace NExternalAPI {
    THTTPRequestSession::TFactory::TRegistrator<THTTPRequestSession> THTTPRequestSession::Registrator(THTTPRequestSession::GetTypeName());

    NFrontend::TScheme IRequestSession::GetScheme(const IBaseServer& server) const {
        return DoGetScheme(server);
    }

    bool IRequestSession::DeserializeFromProto(const NLogisticProto::TRequestSession& proto) {
        return DoDeserializeFromProto(proto);
    }

    void IRequestSession::AppendProblemDetails(const TString& key, const TString& value) {
        DoAppendProblemDetails(key, value);
    }

    void IRequestSession::SetProblemDetails(const NCS::NLogging::TDefaultLogsAccumulator& accumulator) {
        DoSetProblemDetails(accumulator);
    }

    NFrontend::TScheme THTTPRequestSession::DoGetScheme(const IBaseServer& /*server*/) const {
        NFrontend::TScheme result;
        auto& scheme = result.Add<TFSStructure>("http_request_session").SetStructure();
        scheme.Add<TFSString>("request").SetVisual(TFSString::EVisualType::Text).SetRequired(false);
        scheme.Add<TFSString>("response").SetVisual(TFSString::EVisualType::Text).SetRequired(false);
        scheme.Add<TFSString>("problem_details").SetVisual(TFSString::EVisualType::Json).SetRequired(false);
        return result;
    }

    void THTTPRequestSession::DoAppendProblemDetails(const TString& key, const TString& value) {
        if (!ProblemDetails) {
            ProblemDetails = NJson::JSON_ARRAY;
        }
        NJson::TJsonValue jsonItem;
        jsonItem.InsertValue(key, value);
        ProblemDetails->AppendValue(jsonItem);
    }

    void THTTPRequestSession::DoSetProblemDetails(const NCS::NLogging::TDefaultLogsAccumulator& accumulator) {
        ProblemDetails = accumulator.GetJsonReport();
    }

    void THTTPRequestSession::DoSerializeToProto(NLogisticProto::TRequestSession& proto) const {
        auto* mProto = proto.MutableHttpRequestSession();
        if (!!Request && Request->IsDefined()) {
            mProto->SetBRequest(Request->GetStringRobust());
        }
        if (!!Response && Response->IsDefined()) {
            mProto->SetBResponse(Response->GetStringRobust());
        }
        if (!!ProblemDetails && ProblemDetails->IsDefined()) {
            mProto->SetProblemDetails(ProblemDetails->GetStringRobust());
        }
    }

    bool THTTPRequestSession::DoDeserializeFromProto(const NLogisticProto::TRequestSession& proto) {
        if (!proto.HasHttpRequestSession()) {
            TFLEventLog::Error("cannot deserialize http_request_sessoin");
            return false;
        }
        auto& mProto = proto.GetHttpRequestSession();
        {
            TString requestString;
            if (mProto.HasBRequest()) {
                requestString = mProto.GetBRequest();
            } else if (mProto.HasRequest()) {
                requestString = mProto.GetRequest();
            }
            if (!requestString.Empty()) {
                NJson::TJsonValue request;
                if (ReadJsonFastTree(requestString, &request)) {
                    Request = request;
                } else {
                    Request = requestString;
                }
            }
        }
        {
            TString responseString;
            if (mProto.HasBResponse()) {
                responseString = mProto.GetBResponse();
            } else if (mProto.HasResponse()) {
                responseString = mProto.GetResponse();
            }
            if (!responseString.Empty()) {
                NJson::TJsonValue response;
                if (ReadJsonFastTree(responseString, &response)) {
                    Response = response;
                } else {
                    Response = responseString;
                }
            }
        }
        if (mProto.HasProblemDetails()) {
            NJson::TJsonValue problemDetails;
            if (ReadJsonFastTree(mProto.GetProblemDetails(), &problemDetails)) {
                ProblemDetails = problemDetails;
            } else {
                ProblemDetails = mProto.GetProblemDetails();
            }
        }

        return true;
    }

    NFrontend::TScheme TRequestDialog::GetScheme(const IBaseServer& server) {
        NFrontend::TScheme scheme;
        auto& requestSessions = scheme.Add<TFSArray>("request_sessions");
        requestSessions.SetElement<TFSStructure>(TRequestSessionContainer::GetScheme(server)).SetRequired(false);
        requestSessions.SetRequired(false);
        return scheme;
    }

    bool TRequestDialog::DeserializeFromProto(const NLogisticProto::TRequestDialog& proto) {
        for (auto&& i : proto.GetRequestSessions()) {
            TRequestSessionContainer c;
            if (!c.DeserializeFromProto(i)) {
                TFLEventLog::Error("cannot parse request session container");
                return false;
            }
            if (!!c) {
                AddRequestSession(std::move(c));
            }
        }
        return true;
    }

    NLogisticProto::TRequestDialog TRequestDialog::SerializeToProto() const {
        NLogisticProto::TRequestDialog result;
        for (auto&& i : RequestSessions) {
            if (!i){
                continue;
            }
            i.SerializeToProto(*result.AddRequestSessions());
        }
        return result;
    }

    void IRequestSession::SerializeToProto(NLogisticProto::TRequestSession& proto) const {
        DoSerializeToProto(proto);
    }

    THTTPRequestSession& THTTPRequestSession::SetRequest(const TString& requestString) {
        NJson::TJsonValue request;
        if (!ReadJsonFastTree(requestString, &request)) {
            request = NJson::TJsonValue(requestString);
        }
        SetRequest(request);
        return *this;
    }

    THTTPRequestSession& THTTPRequestSession::SetResponse(const TString& responseString) {
        NJson::TJsonValue response;
        if (!ReadJsonFastTree(responseString, &response)) {
            response = NJson::TJsonValue(responseString);
        }
        SetResponse(response);
        return *this;
    }

    TAtomicSharedPtr<THTTPRequestSession> THTTPRequestSession::BuildFailed(const NCS::NLogging::TDefaultLogsAccumulator& accumulator) {
        auto result = MakeAtomicShared<THTTPRequestSession>();
        result->SetProblemDetails(accumulator);
        return result;
    }

    TAtomicSharedPtr<THTTPRequestSession> THTTPRequestSession::BuildFailed(const TString& reason) {
        auto result = MakeAtomicShared<THTTPRequestSession>();
        result->AppendProblemDetails("reason", reason);
        return result;
    }

    TAtomicSharedPtr<THTTPRequestSession> THTTPRequestSession::BuildFromRequestResponse(const NNeh::THttpRequest& request, const NUtil::THttpReply& response) {
        auto result = MakeAtomicShared<THTTPRequestSession>();
        result->SetRequest(request).SetResponse(response);
        return result;
    }

    THTTPRequestSession& THTTPRequestSession::SetRequest(const NNeh::THttpRequest request) {
        SetRequest(request.SerializeToJson());
        return *this;
    }

    THTTPRequestSession& THTTPRequestSession::SetResponse(const NUtil::THttpReply& response) {
        SetResponse(response.Serialize());
        return *this;
    }
} // namespace NExternalAPI
