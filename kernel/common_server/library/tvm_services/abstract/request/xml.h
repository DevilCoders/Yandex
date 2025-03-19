#pragma once
#include "abstract.h"
#include <library/cpp/xml/document/xml-document-decl.h>

namespace NExternalAPI {

    class IHttpRequestWithXMLReport: public IServiceApiHttpRequest {
    private:
        using TBase = IServiceApiHttpRequest;
    public:
        virtual ~IHttpRequestWithXMLReport() {}

        class TXMLResponse: public IResponseByContentType {
        protected:
            virtual TMaybe<EContentType> GetDefaultContentType() const override {
                return EContentType::XML;
            }
            virtual bool DoParseXMLReply(const NXml::TDocument& doc) = 0;
            virtual bool DoParseXMLError(const NXml::TDocument& doc);
            virtual bool DoParseTextError(const TString& reply);
            virtual bool DoParseXML(const NXml::TDocument& doc) override final;
        };

        class TMovableXMLResponse: public IResponse {
        protected:
            virtual bool DoParseXMLReply(NXml::TDocument&& doc) = 0;
            virtual bool DoParseXMLError(NXml::TDocument&& doc);
            virtual bool DoParseTextError(const TString& reply);
            virtual bool DoParseReply(const NUtil::THttpReply& reply) override;
            virtual bool DoParseError(const NUtil::THttpReply& reply) override;
        };
    };
}
