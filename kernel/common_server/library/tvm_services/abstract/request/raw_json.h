#pragma once
#include "json.h"

namespace NExternalAPI {

    class TRawJsonRequest: public IHttpRequestWithJsonReport {
        NJson::TJsonValue PostData;
        TMap<TString, TString> Headers;
        TString Uri;

    public:
        virtual bool BuildHttpRequest(NNeh::THttpRequest& request) const override;

        TRawJsonRequest(const NJson::TJsonValue& postData, const TString& path, const TMap<TString, TString>& headers);

        class TResponse: public TJsonResponse {
            RTLINE_READONLY_ACCEPTOR_DEF(JsonReply, NJson::TJsonValue);
        protected:
            virtual bool DoParseJsonReply(const NJson::TJsonValue& jsonReply) override;
        };
    };
}
