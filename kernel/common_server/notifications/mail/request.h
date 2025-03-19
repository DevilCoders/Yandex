#pragma once

#include <kernel/common_server/library/tvm_services/abstract/abstract.h>
#include <kernel/common_server/notifications/mail/attachment.h>

class TMailRequest: public NExternalAPI::IHttpRequestWithJsonReport {
    CSA_READONLY_DEF(TString, Email);
    CSA_READONLY_DEF(TString, Account);
    CSA_READONLY_DEF(TString, TemplateId);
    CSA_DEFAULT(TMailRequest, TVector<TString>, BccEmails);
    CSA_MAYBE(TMailRequest, TString, Args);
    CSA_DEFAULT(TMailRequest, TVector<TFileAttachment>, Attachments);

public:
    TMailRequest(const TString& email,
                 const TString& account,
                 const TString& templateId);

    virtual bool BuildHttpRequest(NNeh::THttpRequest& request) const override;

    class TResponse: public TJsonResponse {
        CSA_READONLY_DEF(TString, Status);
        CSA_READONLY_DEF(TString, Message);
        CSA_READONLY_DEF(TString, MessageId);
        CSA_READONLY_DEF(TString, TaskId);

    protected:
        virtual bool DoParseJsonReply(const NJson::TJsonValue& json) override;
        virtual bool DoParseJsonError(const NJson::TJsonValue& json) override;

    private:
        Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& json, const bool isError = false);
    };
};
