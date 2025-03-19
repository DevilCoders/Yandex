#pragma once

#include <kernel/common_server/library/tvm_services/abstract/request/xml.h>

class TTextTemplateParams {
    using TParams = TMap<TString, TString>;
    CSA_DEFAULT(TTextTemplateParams, TParams, Params);
public:

    NJson::TJsonValue SerializeToJson() const {
        NJson::TJsonValue map = NJson::JSON_MAP;
        for (const auto& [key, value]: Params) {
            map.InsertValue(key, value);
        }
        return map;
    }
};

class TSmsRequest: public NExternalAPI::IHttpRequestWithXMLReport {
    CSA_READONLY_DEF(TString, Sender);
    CSA_READONLY_DEF(TString, Text);
    CSA_DEFAULT(TSmsRequest, TString, UID);
    CSA_DEFAULT(TSmsRequest, TString, Phone);
    CSA_DEFAULT(TSmsRequest, TString, PhoneId);
    CSA_DEFAULT(TSmsRequest, TString, Route);
    CSA_DEFAULT(TSmsRequest, TString, Udh);
    CSA_DEFAULT(TSmsRequest, TString, Identity);
    CSA_DEFAULT(TSmsRequest, TTextTemplateParams, TextTemplateParams);

public:
    TSmsRequest(const TString& sender, const TString& text);

    virtual bool BuildHttpRequest(NNeh::THttpRequest& request) const override;

    class TResponse: public TXMLResponse {
        CSA_READONLY_DEF(TString, SmsId);
        CSA_READONLY_DEF(TString, ErrorMessage);
        CSA_READONLY_DEF(TString, ErrorCode);

    protected:
        virtual bool DoParseXMLReply(const NXml::TDocument& doc) override;
        virtual bool DoParseXMLError(const NXml::TDocument& doc) override;

    private:
        Y_WARN_UNUSED_RESULT bool DeserializeFromXML(const NXml::TDocument& doc, const bool isError = false);
    };
};
