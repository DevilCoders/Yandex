#pragma once
#include "abstract.h"

namespace NExternalAPI {

    class IHttpRequestWithZipReport: public IServiceApiHttpRequest {
    private:
        using TBase = IServiceApiHttpRequest;
    public:
        virtual ~IHttpRequestWithZipReport() {}

        class TZipResponse: public IResponseByContentType {
        protected:
            virtual TMaybe<EContentType> GetDefaultContentType() const override {
                return EContentType::Zip;
            }
            virtual bool DoParseZipReply(const TStringBuf data) = 0;
            virtual bool DoParseZipError(const TStringBuf /*data*/) {
                return true;
            }
            virtual bool DoParseZip(const TStringBuf data) override final;
        };
    };
}
