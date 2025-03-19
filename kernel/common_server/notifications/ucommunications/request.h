#pragma once

#include <kernel/common_server/library/tvm_services/abstract/abstract.h>
#include <kernel/common_server/library/tvm_services/abstract/request/json.h>

class TSendSmsRequest: public NExternalAPI::IHttpRequestWithJsonReport {
    CSA_DEFAULT(TSendSmsRequest, TString, PhoneNumber)
    CSA_DEFAULT(TSendSmsRequest, TString, PhoneId)
    CSA_DEFAULT(TSendSmsRequest, TString, Message)
    CSA_DEFAULT(TSendSmsRequest, TString, Intent)
    CSA_DEFAULT(TSendSmsRequest, TString, Sender)
public:
    bool BuildHttpRequest(NNeh::THttpRequest& request) const override;

    class TResponse: public TJsonResponse {
    protected:
        virtual bool DoParseJsonReply(const NJson::TJsonValue& /*jsonReply*/) override {
            return true;
        }
    };
};
