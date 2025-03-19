#include <kernel/common_server/library/startrek/client.h>

#include <library/cpp/json/json_reader.h>
#include <kernel/common_server/util/json_processing.h>

void TStartrekRequestCallback::ProcessResponseData() {
    if (!IsRequestSuccessful(Code)) {
        if (Result.Has("errorMessages") && Result["errorMessages"].IsArray()) {
            if (ErrorMessage) {
                ErrorMessage += "; ";
            }
            ErrorMessage += Result["errorMessages"].GetStringRobust();
        }
        if (Result.Has("errors") && Result["errors"].IsMap()) {
            if (ErrorMessage) {
                ErrorMessage += "; ";
            }
            ErrorMessage += Result["errors"].GetStringRobust();
        }
        if (!ErrorMessage) {
            ErrorMessage = "Unknown error";
        }
    }
}

bool TStartrekClient::GetIssueInfo(const TString& issueName, TTicket& result) const {
    NJson::TJsonValue rawResult;
    if (!SendRequest(EStartrekOperationType::GetIssue, CreateGetIssueRequest(issueName), rawResult)) {
        return false;
    }
    return result.DeserializeFromJson(rawResult);
}

bool TStartrekClient::CreateIssue(const TTicket& content, TTicket& result) const {
    NJson::TJsonValue rawResult;
    if (!SendRequest(EStartrekOperationType::CreateIssue, CreateMakeIssueRequest(content.SerializeToJson()), rawResult)) {
        return false;
    }
    return result.DeserializeFromJson(rawResult);
}

bool TStartrekClient::PatchIssue(const TString& issueName, const TTicket& update, TTicket& result) const {
    NJson::TJsonValue rawResult;
    if (!SendRequest(EStartrekOperationType::PatchIssue, CreatePatchIssueRequest(issueName, update.SerializeToJson()), rawResult)) {
        return false;
    }
    return result.DeserializeFromJson(rawResult);
}

bool TStartrekClient::AddComment(const TString& issueName, const TComment& comment) const {
    TComment result;
    return AddComment(issueName, comment, result);
}

bool TStartrekClient::AddComment(const TString& issueName, const TComment& comment, TComment& result) const {
    NJson::TJsonValue rawResult;  // comment meta info or error info
    if (!SendRequest(EStartrekOperationType::AddComment, CreateAddCommentRequest(issueName, comment.SerializeToJson()), rawResult)) {
        return false;
    }
    return result.DeserializeFromJson(rawResult);
}

bool TStartrekClient::GetComment(const TString& issueName, const TString& commentId, TComment& result) const {
    NJson::TJsonValue rawResult;
    if (!SendRequest(EStartrekOperationType::GetComment, CreateGetCommentRequest(issueName, commentId), rawResult)) {
        return false;
    }
    return result.DeserializeFromJson(rawResult);
}

bool TStartrekClient::GetAllComments(const TString& issueName, TComments& result) const {
    NJson::TJsonValue rawResult;
    if (!SendRequest(EStartrekOperationType::GetAllComments, CreateGetAllCommentsRequest(issueName), rawResult)) {
        return false;
    }
    for (const auto& item : rawResult.GetArray()) {
        TComment comment;
        if (!comment.DeserializeFromJson(item)) {
            return false;
        }
        result.push_back(std::move(comment));
    }
    return true;
}

bool TStartrekClient::DeleteComment(const TString& issueName, const TString& commentId) const {
    NJson::TJsonValue rawResult;
    return SendRequest(EStartrekOperationType::DeleteComment, CreateDeleteCommentRequest(issueName, commentId), rawResult);
}

bool TStartrekClient::GetTransitions(const TString& issueName, TTransitions& result) const {
    NJson::TJsonValue transitions;
    if (!SendRequest(EStartrekOperationType::GetTransitions, CreateGetTransitionsRequest(issueName), transitions)) {
        return false;
    }

    if (!transitions.IsArray()) {
        errors.AddMessage(__LOCATION__, "transitions have unexpected format");
        return false;
    }

    for (const auto& rawTransition : transitions.GetArray()) {
        TTransition transition;
        if (!transition.DeserializeFromJson(rawTransition)) {
            return false;
        }
        result.push_back(std::move(transition));
    }

    return true;
}

bool TStartrekClient::ExecuteTransition(const TString& issueName, const TString& transition, const TTicket& update) const {
    NJson::TJsonValue result;  // updated transitions or error info
    return SendRequest(EStartrekOperationType::ExecuteTransition, CreateExecuteTransitionRequest(issueName, transition, update.SerializeToJson()), result);
}

NNeh::THttpRequest TStartrekClient::CreateGetIssueRequest(const TString& issueName) const {
    return CreateCommonRequest("v2/issues/" + issueName);
}

NNeh::THttpRequest TStartrekClient::CreateMakeIssueRequest(const NJson::TJsonValue& content) const {
    return CreateCommonRequest("v2/issues/", ERequestMethod::POST, content, ERequestContentType::Json);
}

NNeh::THttpRequest TStartrekClient::CreatePatchIssueRequest(const TString& issueName, const NJson::TJsonValue& update) const {
    return CreateCommonRequest("v2/issues/" + issueName, ERequestMethod::PATCH, update, ERequestContentType::Json);
}

NNeh::THttpRequest TStartrekClient::CreateAddCommentRequest(const TString& issueName, const NJson::TJsonValue& commentData) const {
    return CreateCommonRequest("v2/issues/" + issueName + "/comments", ERequestMethod::POST, commentData, ERequestContentType::Json);
}

NNeh::THttpRequest TStartrekClient::CreateGetCommentRequest(const TString& issueName, const TString& commentId) const {
    return CreateCommonRequest("v2/issues/" + issueName + "/comments/" + commentId);
}

NNeh::THttpRequest TStartrekClient::CreateGetAllCommentsRequest(const TString& issueName) const {
    return CreateCommonRequest("v2/issues/" + issueName + "/comments");
}

NNeh::THttpRequest TStartrekClient::CreateDeleteCommentRequest(const TString& issueName, const TString& commentId) const {
    return CreateCommonRequest("v2/issues/" + issueName + "/comments/" + commentId, ERequestMethod::DELETE, {}, ERequestContentType::Json);
}

NNeh::THttpRequest TStartrekClient::CreateGetTransitionsRequest(const TString& issueName) const {
    return CreateCommonRequest("v2/issues/" + issueName + "/transitions");
}

NNeh::THttpRequest TStartrekClient::CreateExecuteTransitionRequest(const TString& issueName, const TString& transition, const NJson::TJsonValue& data) const {
    return CreateCommonRequest("v2/issues/" + issueName + "/transitions/" + transition + "/_execute", ERequestMethod::POST, data, ERequestContentType::Json);
}
