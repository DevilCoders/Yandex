#pragma once

#include "objects.h"
#include <kernel/common_server/library/tvm_services/abstract/abstract.h>
#include <kernel/common_server/util/json_processing.h>

namespace NCS::NStartrek {

    class TCommentResponse: public NExternalAPI::IHttpRequestWithJsonReport::TJsonResponse {
        CSA_READONLY_DEF(TComment, Comment);
    protected:
        bool DoParseJsonReply(const NJson::TJsonValue& json) override;
        bool IsReplyCodeSuccess(const i32 code) const override {
            return code == 200 || code == 201;
        }
    };

    class TCommentArrayResponse: public NExternalAPI::IHttpRequestWithJsonReport::TJsonResponse {
        CSA_READONLY_DEF(TVector<TComment>, Comments);
    protected:
        bool DoParseJsonReply(const NJson::TJsonValue& json) override;
    };

    class TAddCommentStartrekRequest: public NExternalAPI::IHttpRequestWithJsonReport {
        CSA_READONLY_DEF(TString, Text);
        CSA_READONLY_DEF(TString, Issue);
        CSA_DEFAULT(TAddCommentStartrekRequest, TVector<TString>, SummoneeIds);
    public:
        TAddCommentStartrekRequest(const TString& issue, const TString& text)
                : Text(text), Issue(issue)
        {}
        virtual bool BuildHttpRequest(NNeh::THttpRequest& request) const override;
        using TResponse = TCommentResponse;
    };

    class TGetCommentStartrekRequest: public NExternalAPI::IHttpRequestWithJsonReport {
        CSA_READONLY_DEF(TString, Issue);
        CSA_READONLY_DEF(TString, CommentId);
    public:
        TGetCommentStartrekRequest(const TString& issue, const TString& commentId)
            : Issue(issue), CommentId(commentId)
        {}
        virtual bool BuildHttpRequest(NNeh::THttpRequest& request) const override;
        using TResponse = TCommentResponse;
    };

    class TGetAllCommentsStartrekRequest: public NExternalAPI::IHttpRequestWithJsonReport {
        CSA_READONLY_DEF(TString, Issue);
    public:
        TGetAllCommentsStartrekRequest(const TString& issue)
            : Issue(issue)
        {}
        virtual bool BuildHttpRequest(NNeh::THttpRequest& request) const override;
        using TResponse = TCommentArrayResponse;
    };
}
