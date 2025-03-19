#pragma once

#include "objects.h"
#include <kernel/common_server/library/tvm_services/abstract/abstract.h>
#include <kernel/common_server/util/json_processing.h>

namespace NCS::NStartrek {

    class TIssueResponse: public NExternalAPI::IHttpRequestWithJsonReport::TJsonResponse {
        CSA_READONLY_DEF(TDetailedIssue, DetailedIssue);
    protected:
        bool DoParseJsonReply(const NJson::TJsonValue& json) override;
        bool IsReplyCodeSuccess(const i32 code) const override {
            return code == 200 || code == 201;
        }
    };

    class TIssueArrayResponse: public NExternalAPI::IHttpRequestWithJsonReport::TJsonResponse {
        CSA_READONLY_DEF(TVector<TDetailedIssue>, DetailedIssues);
    protected:
        bool DoParseJsonReply(const NJson::TJsonValue& json) override;
    };

    class TNewIssueStartrekRequest: public NExternalAPI::IHttpRequestWithJsonReport, public TBaseIssue {
        CSA_MUTABLE_DEF(TNewIssueStartrekRequest, TString, Unique);
        CSA_DEFAULT(TNewIssueStartrekRequest, TString, Queue);
    public:
        TNewIssueStartrekRequest(const TString& summary, const TString& queue, const TString& description = {});

        class TResponse: public TIssueResponse {
            using TBase = TIssueResponse;
            bool IsReplyCodeSuccess(const i32 code) const override;
        };
        virtual bool BuildHttpRequest(NNeh::THttpRequest& request) const override;
    };

    class TGetIssueStartrekRequest: public NExternalAPI::IHttpRequestWithJsonReport {
        CSA_READONLY_DEF(TString, Issue);
    public:
        TGetIssueStartrekRequest(const TString& issue)
            : Issue(issue)
        {}
        virtual bool BuildHttpRequest(NNeh::THttpRequest& request) const override;
        using TResponce = TIssueResponse;
    };

    class TPatchIssueStartrekRequest: public NExternalAPI::IHttpRequestWithJsonReport {
        CSA_READONLY_DEF(TString, Issue);
        CSA_READONLY_DEF(TBaseIssue, UpdateIssue);
    public:
        TPatchIssueStartrekRequest(const TString& issue, const TBaseIssue& update)
                : Issue(issue), UpdateIssue(update)
        {}
        virtual bool BuildHttpRequest(NNeh::THttpRequest& request) const override;
        using TResponce = TIssueResponse;
    };

    class TSearchIssueRequest: public NExternalAPI::IHttpRequestWithJsonReport {
        CSA_READONLY_DEF(TString, Query /* https://wiki.yandex-team.ru/tracker/vodstvo/query/ */);
    public:
        TSearchIssueRequest(const TString& query)
                : Query(query)
        {}
        virtual bool BuildHttpRequest(NNeh::THttpRequest& request) const override;
        using TResponce = TIssueArrayResponse;
    };
}
