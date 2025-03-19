#include "issue_requests.h"

namespace NCS::NStartrek {

    bool TIssueResponse::DoParseJsonReply(const NJson::TJsonValue& json) {
        return DetailedIssue.DeserializeFromJson(json);
    }

    bool TIssueArrayResponse::DoParseJsonReply(const NJson::TJsonValue& json) {
        return TJsonProcessor::ReadObjectsContainer(json, DetailedIssues);
    }

    TNewIssueStartrekRequest::TNewIssueStartrekRequest(const TString& summary, const TString& queue, const TString& description)
            :  Queue(queue) {
        TBaseIssue::SetSummary(summary);
        TBaseIssue::SetDescription(description);
    }

    bool TNewIssueStartrekRequest::TResponse::IsReplyCodeSuccess(const i32 code) const {
        return code == 200  || code == 201 || code == 409;
    }

    bool TNewIssueStartrekRequest::BuildHttpRequest(NNeh::THttpRequest& request) const {
        request.SetUri("v2/issues/");
        NJson::TJsonValue postData = TBaseIssue::SerializeToJson();
        JWRITE(postData, "queue", Queue);
        if (!Unique) {
            Unique = ToString(::hash<TString>{}(Summary + Queue));
        }
        JWRITE(postData, "unique", Unique);
        request.SetPostData(postData);
        return true;
    }


    bool TPatchIssueStartrekRequest::BuildHttpRequest(NNeh::THttpRequest& request) const {
        if (!Issue) {
            return false;
        }
        request.SetUri("v2/issues/" + Issue);
        SetSignalId("v2/issues/");
        request.SetPostData(UpdateIssue.SerializeToJson(), "PATCH");
        return true;
    }

    bool TSearchIssueRequest::BuildHttpRequest(NNeh::THttpRequest& request) const {
        if (!Query) {
            return false;
        }
        request.SetUri("v2/issues/_search");
        SetSignalId("v2/issues/");
        NJson::TJsonValue result = NJson::TJsonMap();
        JWRITE(result, "query", Query)
        request.SetPostData(result);
        return true;
    }
}
