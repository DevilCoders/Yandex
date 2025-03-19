#include "xml.h"

namespace NExternalAPI {

    bool IHttpRequestWithXMLReport::TXMLResponse::DoParseXML(const NXml::TDocument& doc) {
        if (IsReplyCodeSuccess()) {
            return DoParseXMLReply(doc);
        } else {
            return DoParseXMLError(doc);
        }
    }

    bool IHttpRequestWithXMLReport::TXMLResponse::DoParseXMLError(const NXml::TDocument& doc) {
        TFLEventLog::Error("xml response error")("xml", doc.ToString());
        return true;
    }

    bool IHttpRequestWithXMLReport::TXMLResponse::DoParseTextError(const TString& reply) {
        TFLEventLog::Error("text response error")("reply", reply);
        return true;
    }

    bool IHttpRequestWithXMLReport::TMovableXMLResponse::DoParseReply(const NUtil::THttpReply& reply) {
        return DoParseXMLReply(NXml::TDocument(reply.Content(), NXml::TDocument::String));
    }

    bool IHttpRequestWithXMLReport::TMovableXMLResponse::DoParseError(const NUtil::THttpReply& reply) {
        if (reply.Content().StartsWith("<?xml")){
            try {
                auto xml = NXml::TDocument(reply.Content(), NXml::TDocument::String);
                return DoParseXMLError(std::move(xml));
            } catch (...) {
                TFLEventLog::Error("exception occured while parsing xml")("exception", CurrentExceptionMessage());
            }
        }
        return DoParseTextError(reply.Content());
    }

    bool IHttpRequestWithXMLReport::TMovableXMLResponse::DoParseXMLError(NXml::TDocument&& doc) {
        TFLEventLog::Error("xml response error")("xml", doc.ToString());
        return true;
    }

    bool IHttpRequestWithXMLReport::TMovableXMLResponse::DoParseTextError(const TString& reply) {
        TFLEventLog::Error("text response error")("reply", reply);
        return true;
    }
} // namespace NExternalAPI
