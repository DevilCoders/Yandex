#pragma once

#include <kernel/common_server/library/startrek/config.h>
#include <kernel/common_server/library/startrek/logger.h>
#include <kernel/common_server/library/startrek/entity.h>

#include <kernel/common_server/library/async_impl/client.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/generic/cast.h>
#include <util/string/cast.h>

class TStartrekRequestCallback : public TRequestCallback<EStartrekOperationType, TStartrekLogger> {
    using TBase = TRequestCallback<EStartrekOperationType, TStartrekLogger>;

public:
    TStartrekRequestCallback(NJson::TJsonValue& result, ui32& code, TString& errorMessage, const EStartrekOperationType type, const TStartrekLogger& logger)
        : TBase(result, code, errorMessage, type, logger)
    {
    }

    virtual void ProcessResponseData() override;
};

class TStartrekClient : public TRequestClient<TStartrekClientConfig, TStartrekLogger, EStartrekOperationType, TStartrekRequestCallback> {
    using TBase = TRequestClient<TStartrekClientConfig, TStartrekLogger, EStartrekOperationType, TStartrekRequestCallback>;

public:
    using TTicket = TStartrekTicket;
    using TComment = TStartrekComment;
    using TComments = TVector<TStartrekComment>;
    using TTransition = TStartrekTransition;
    using TTransitions = TVector<TTransition>;

    TStartrekClient(const TStartrekClientConfig& config)
        : TBase(config, "startrek_api")
    {
    }

    bool GetIssueInfo(const TString& issueName, TTicket& result) const;
    bool CreateIssue(const TTicket& content,  TTicket& result) const;
    bool PatchIssue(const TString& issueName, const TTicket& update,  TTicket& result) const;

    bool AddComment(const TString& issueName, const TComment& comment) const;
    bool AddComment(const TString& issueName, const TComment& comment, TComment& result) const;
    bool GetComment(const TString& issueName, const TString& commentId, TComment& result) const;
    bool GetAllComments(const TString& issueName, TComments& result) const;
    bool DeleteComment(const TString& issueName, const TString& commentId) const;

    bool GetTransitions(const TString& issueName, TTransitions& result) const;
    bool ExecuteTransition(const TString& issueName, const TString& transition, const TTicket& update = Default<TTicket>()) const;

private:
    NNeh::THttpRequest CreateGetIssueRequest(const TString& issueName) const;
    NNeh::THttpRequest CreateMakeIssueRequest(const NJson::TJsonValue& content) const;
    NNeh::THttpRequest CreatePatchIssueRequest(const TString& issueName, const NJson::TJsonValue& update) const;
    NNeh::THttpRequest CreateAddCommentRequest(const TString& issueName, const NJson::TJsonValue& commentData) const;
    NNeh::THttpRequest CreateGetCommentRequest(const TString& issueName, const TString& commentId) const;
    NNeh::THttpRequest CreateGetAllCommentsRequest(const TString& issueName) const;
    NNeh::THttpRequest CreateDeleteCommentRequest(const TString& issueName, const TString& commentId) const;
    NNeh::THttpRequest CreateGetTransitionsRequest(const TString& issueName) const;
    NNeh::THttpRequest CreateExecuteTransitionRequest(const TString& issueName, const TString& transition, const NJson::TJsonValue& data = {}) const;
};
