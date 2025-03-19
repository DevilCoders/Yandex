#include "comment_requests.h"

namespace NCS::NStartrek {
    bool TCommentResponse::DoParseJsonReply(const NJson::TJsonValue& json) {
        return Comment.DeserializeFromJson(json);
    }

    bool TCommentArrayResponse::DoParseJsonReply(const NJson::TJsonValue& json) {
        return TJsonProcessor::ReadObjectsContainer(json, Comments);
    }

    bool TAddCommentStartrekRequest::BuildHttpRequest(NNeh::THttpRequest& request) const {
        if (!Issue || !Text) {
            return false;
        }
        request.SetUri("v2/issues/" + Issue + "/comments/");
        SetSignalId("v2/issues/");
        NJson::TJsonMap postData;
        TJsonProcessor::Write(postData, "text", Text);
        TJsonProcessor::WriteContainerArray(postData, "summonees", SummoneeIds, false);
        return true;
    }

    bool TGetCommentStartrekRequest::BuildHttpRequest(NNeh::THttpRequest& request) const {
        request.SetUri("v2/issues/" + Issue + "/comments/" + CommentId);
        SetSignalId("v2/issues/");
        return true;
    }

    bool TGetAllCommentsStartrekRequest::BuildHttpRequest(NNeh::THttpRequest& request) const {
        request.SetUri("v2/issues/" + Issue + "/comments/");
        SetSignalId("v2/issues/");
        return true;
    }

}
