#pragma once
#include <library/cpp/http/io/headers.h>
#include <library/cpp/json/writer/json_value.h>
#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/util/accessor.h>

namespace NXml {
    class TDocument;
}

namespace NExternalAPI {

    enum class EContentType {
        XML /* "text/xml" */,
        Json /* "text/json" */,
        Multipart /* "multipart" */,
        Text /* "text/plain" */,
        Binary /* "binary" */,
        Zip /* zip */,
        Undefined /* "undefined" */,
    };

    class TContentPartContext {
    private:
        CS_ACCESS(TContentPartContext, EContentType, ContentType, EContentType::Undefined);
        CSA_DEFAULT(TContentPartContext, TString, PartName);
        const THttpHeaders& Headers;
    public:
        TContentPartContext(const THttpHeaders& headers)
            : Headers(headers) {

        }

        const THttpHeaders& GetHeaders() const {
            return Headers;
        }
    };

    class IParserCallbackContext {
    protected:
        virtual bool DoStart(const TContentPartContext& context) = 0;
        virtual bool DoFinish() = 0;
        virtual bool DoParseXML(const NXml::TDocument& doc) = 0;
        virtual bool DoParseJson(const NJson::TJsonValue& jsonInfo) = 0;
        virtual bool DoParseText(const TStringBuf text) = 0;
        virtual bool DoParseBinary(const TStringBuf data) = 0;
        virtual bool DoParseZip(const TStringBuf data) = 0;
    public:
        virtual TMaybe<EContentType> GetDefaultContentType() const {
            return Nothing();
        }
        virtual bool Start(const TContentPartContext& context) {
            return DoStart(context);
        }
        virtual bool Finish() {
            return DoFinish();
        }
        virtual bool ParseXML(const NXml::TDocument& doc) {
            return DoParseXML(doc);
        }
        virtual bool ParseJson(const NJson::TJsonValue& jsonInfo) {
            return DoParseJson(jsonInfo);
        }
        virtual bool ParseText(const TStringBuf text) {
            return DoParseText(text);
        }
        virtual bool ParseBinary(const TStringBuf data) {
            return DoParseBinary(data);
        }
        virtual bool ParseZip(const TStringBuf data) {
            return DoParseZip(data);
        }
    };

    class TBaseParserCallbackContext: public IParserCallbackContext {
    public:
        virtual bool DoStart(const TContentPartContext& /*context*/) override {
            return true;
        }
        virtual bool DoFinish() override {
            return true;
        }
        virtual bool DoParseXML(const NXml::TDocument& /*doc*/) override {
            TFLEventLog::Error("unexpected xml content for response");
            return false;
        }
        virtual bool DoParseJson(const NJson::TJsonValue& /*jsonInfo*/) override {
            TFLEventLog::Error("unexpected json content for response");
            return false;
        }
        virtual bool DoParseText(const TStringBuf /*text*/) override {
            TFLEventLog::Error("unexpected text content for response");
            return false;
        }
        virtual bool DoParseBinary(const TStringBuf /*data*/) override {
            TFLEventLog::Error("unexpected binary content for response");
            return false;
        }
        virtual bool DoParseZip(const TStringBuf /*data*/) override {
            TFLEventLog::Error("unexpected unzip content for response");
            return false;
        }
    };

    class TContentTypeParser {
    private:
        CSA_READONLY_DEF(TString, Boundary);
        CSA_READONLY(EContentType, ContentType, EContentType::Undefined);
    public:
        bool Parse(const TString& contentType);
    };

    class TParser {
    private:
        CSA_DEFAULT(TParser, TString, PartName);
        CS_ACCESS(TParser, ui32, ParsingLevel, 0);

        const TStringBuf Content;
        const THttpHeaders Headers;
        IParserCallbackContext& CallbackContext;

        bool ParseMultipart(const TString& boundary);
        bool ParseXML();
        bool ParseJson();
        bool ParseText();
        bool ParseBinary();
        bool ParseZip();
    public:
        TParser(const TStringBuf content, const THttpHeaders& headers, IParserCallbackContext& callbackContext)
            : Content(content)
            , Headers(headers)
            , CallbackContext(callbackContext) {

        }

        bool Parse() noexcept;
    };
}

