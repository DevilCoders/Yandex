#include "send_request.h"

namespace NCS {
    namespace NPropositions {

        IProposedAction::TFactory::TRegistrator<TSendRequestAction> RegistratorSendRequest(TSendRequestAction::GetTypeName());

        NJson::TJsonValue TSendRequestAction::SerializeToJson() const {
            NJson::TJsonValue result = NJson::JSON_MAP;
            JWRITE(result, "request_name", RequestName);
            JWRITE(result, "sender_name", SenderName);
            JWRITE(result, "request_uri", RequestURI);
            JWRITE(result, "request_type", RequestType);
            JWRITE(result, "request_body", RequestBody);
            TJsonProcessor::WriteMap(result, "request_headers", RequestHeaders);
            return result;
        }

        bool TSendRequestAction::DeserializeFromJson(const NJson::TJsonValue& jsonData) {
            JREAD_STRING_OPT(jsonData, "request_name", RequestName);
            JREAD_STRING_OPT(jsonData, "sender_name", SenderName);
            JREAD_STRING_OPT(jsonData, "request_uri", RequestURI);
            JREAD_STRING_OPT(jsonData, "request_type", RequestType);
            JREAD_STRING_OPT(jsonData, "request_body", RequestBody);
            [[maybe_unused]] bool b = TJsonProcessor::ReadMap(jsonData, "request_headers", RequestHeaders);
            return true;
        }

        NCS::NScheme::TScheme TSendRequestAction::GetScheme(const IBaseServer& server) const {
            NCS::NScheme::TScheme scheme;
            scheme.Add<TFSString>("request_name", "Название пропозишена").SetRequired(true);
            scheme.Add<NScheme::TFSVariants>("sender_name", "SenderId").SetVariants(server.GetAbstractExternalAPINames()).SetRequired(true);
            scheme.Add<TFSString>("request_uri", "URL запроса").SetDefault("background/info").SetRequired(true);
            scheme.Add<NScheme::TFSVariants>("request_type", "Тип запроса").SetVariants({"POST", "PUT", "PATCH", "GET"}).SetRequired(true);
            scheme.Add<TFSString>("request_body", "Body запроса").SetRequired(true);
            auto& fields = scheme.Add<TFSArray>("request_headers", "Хэдеры запроса").SetElement<NCS::NScheme::TScheme>();
            fields.Add<TFSString>("key", "Ключ");
            fields.Add<TFSString>("value", "Значение");
            return scheme;
        }

        TString TSendRequestAction::GetObjectId() const {
            return RequestName;
        }

        TString TSendRequestAction::GetCategoryId() const {
            return GetTypeName();
        }

        TString TSendRequestAction::GetClassName() const {
            return GetTypeName();
        }

        bool TSendRequestAction::IsActual(const IBaseServer& server) const {
            Y_UNUSED(server);
            return true;
        }

        TString TSendRequestAction::GetResult() const {
            return RequestResult;
        }

        bool TSendRequestAction::DoExecute(const TString& userId, const IBaseServer& server) const {
            Y_UNUSED(userId);
            auto sender = server.GetSenderPtr(GetSenderName());
            if (!sender) {
                return false;
            }
            NNeh::THttpRequest request;
            request.SetUri(GetRequestURI());
            const TString& requestType = GetRequestType();
            request.SetRequestType(requestType);
            if (requestType == "POST" || requestType == "PUT" || requestType == "PATCH") {
                request.SetPostData(GetRequestBody(), requestType);
            }
            for (const auto& [name, value] : RequestHeaders) {
                request.AddHeader(name, value);
            }
            const auto response = sender->SendRequest(NExternalAPI::TServiceApiHttpDirectRequest(request));
            SetRequestResult(response.GetReply().GetDebugReply());
            return true;
        }

        bool TSendRequestAction::TuneAction(NCS::TEntitySession& session, IReplyContext::TPtr requestContext) const {
            if (!requestContext) {
                session.Error("requestContext is nullptr");
                return false;
            }
            auto actionHeaders = GetRequestHeaders();
            for (auto& actionHeader : actionHeaders) {
                if (actionHeader.second == "$FromIncomingHeaders") {
                    const auto requestHeaderPtr = requestContext->GetBaseRequestData().HeadersIn().FindPtr(actionHeader.first);
                    if (!requestHeaderPtr) {
                        session.Error("no such header with key: " + actionHeader.first);
                        return false;
                    }
                    actionHeader.second = *requestHeaderPtr;
                }
            }
            SetRequestHeaders(actionHeaders);
            return true;
        }

    }
}
