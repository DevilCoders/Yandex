#pragma once
#include "abstract.h"

namespace NExternalAPI {

    class IHttpRequestWithJsonReport: public IServiceApiHttpRequest {
    private:
        using TBase = IServiceApiHttpRequest;
    public:
        virtual ~IHttpRequestWithJsonReport() {}

        class TJsonResponse: public IResponseByContentType {
        protected:
            virtual TMaybe<EContentType> GetDefaultContentType() const override {
                return EContentType::Json;
            }
            virtual bool DoParseJsonReply(const NJson::TJsonValue& doc) = 0;
            virtual bool DoParseJsonError(const NJson::TJsonValue& /*doc*/) {
                return true;
            }
            virtual bool DoParseJson(const NJson::TJsonValue& jsonInfo) override final;
        public:
        };
    };
}
