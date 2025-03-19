#pragma once
#include "abstract.h"

namespace NXml {
    class TDocument;
};

namespace NExternalAPI {
    class TServiceApiHttpDirectRequest: public IServiceApiHttpRequest {
        TMaybe<NNeh::THttpRequest> RawRequest;
    public:
        TServiceApiHttpDirectRequest() = default;
        TServiceApiHttpDirectRequest(const NNeh::THttpRequest& request)
            : RawRequest(request) {
        }

        virtual bool BuildHttpRequest(NNeh::THttpRequest& request) const override;

        class TResponse: public IServiceApiHttpRequest::IResponse {
        public:
            CSA_READONLY_DEF(NUtil::THttpReply, Reply);
        protected:
            bool DoParseReply(const NUtil::THttpReply& reply) override;
            virtual bool DoParseError(const NUtil::THttpReply& reply) override {
                return DoParseReply(reply);
            }
        };
    };
}
