#include "ct_parser.h"
#include <library/cpp/xml/document/xml-document.h>
#include <library/cpp/json/json_reader.h>
namespace NExternalAPI {

    bool TParser::ParseMultipart(const TString& boundary) {
        TStringBuf sb = Content;
        if (!sb.SkipPrefix("\r\n") && !sb.SkipPrefix("\n")) {
            TFLEventLog::Notice("reply is not correct multipart message: empty line not found before content");
        }
        while (sb.size()) {
            if (!sb.SkipPrefix("--") || !sb.SkipPrefix(boundary)) {
                TFLEventLog::Error("incorrect format multipart message content");
                return false;
            }
            if (sb.StartsWith("--")) {
                return true;
            }
            if (!sb.SkipPrefix("\r\n") && !sb.SkipPrefix("\n")) {
                TFLEventLog::Error("incorrect format multipart message content on reading headers");
                return false;
            }
            THttpHeaders headers;
            TStringBuf partName;
            while (sb.size()) {
                TStringBuf header;
                TStringBuf content;
                if (!sb.TrySplit("\r\n", header, content) && !sb.TrySplit('\n', header, content)) {
                    TFLEventLog::Error("incorrect format multipart message content on reading headers");
                    return false;
                }
                sb = content;
                if (!header) {
                    break;
                }
                TStringBuf hName;
                TStringBuf hValue;
                if (!header.TrySplit(':', hName, hValue)) {
                    TFLEventLog::Error("incorrect header format (name : value)")("header", header);
                    return false;
                }
                headers.AddHeader(TString(hName.data(), hName.size()), hValue);
                if (hName == "Content-Disposition") {
                    TVector<TStringBuf> fields = StringSplitter(hValue).SplitBySet("; ").SkipEmpty().ToList<TStringBuf>();
                    for (auto&& i : fields) {
                        if (i.StartsWith("name=")) {
                            partName = i.substr(5);
                            while (partName.StartsWith("\"")) {
                                partName.Skip(1);
                            }
                            while (partName.EndsWith("\"")) {
                                partName.Chop(1);
                            }
                            break;
                        }
                    }
                }
            }

            const size_t bIndexNext = sb.find(TString("--") + boundary);
            if (bIndexNext == TString::npos) {
                TFLEventLog::Error("cannot read closed boundary");
                return false;
            }

            TStringBuf sbSubContent = sb.substr(0, bIndexNext);
            TParser partParser(sbSubContent, headers, CallbackContext);
            partParser.SetPartName(TString(partName.data(), partName.size())).SetParsingLevel(ParsingLevel + 1);
            if (!partParser.Parse()) {
                TFLEventLog::Error("cannot parse part")("part_name", partName)("content", sbSubContent);
                return false;
            }
            sb.Skip(bIndexNext);
        }
        TFLEventLog::Error("incorrect multipart format finish");
        return false;
    }

    bool TParser::Parse() noexcept {
        auto* cType = Headers.FindHeader("Content-Type");
        TString contentType;
        if (!cType) {
            if (ParsingLevel == 0 && !!CallbackContext.GetDefaultContentType()) {
                contentType = ::ToString(*CallbackContext.GetDefaultContentType());
            } else {
                TFLEventLog::Error("segment hasn't information about content type");
                return false;
            }
        } else {
            contentType = cType->Value();
        }
        try {
            TContentTypeParser ctParser;
            if (!ctParser.Parse(contentType)) {
                TFLEventLog::Error("cannot parse content type info")("content_type", cType->Value());
                return false;
            }
            TContentPartContext context(Headers);
            context.SetContentType(ctParser.GetContentType());
            context.SetPartName(GetPartName());
            if (!CallbackContext.Start(context)) {
                return false;
            }
            bool result = false;
            switch (ctParser.GetContentType()) {
                case EContentType::Multipart:
                    result = ParseMultipart(ctParser.GetBoundary());
                    break;
                case EContentType::XML:
                    result = ParseXML();
                    break;
                case EContentType::Json:
                    result = ParseJson();
                    break;
                case EContentType::Text:
                    result = ParseText();
                    break;
                case EContentType::Binary:
                    result = ParseBinary();
                    break;
                case EContentType::Zip:
                    result = ParseZip();
                    break;
                case EContentType::Undefined:
                    break;
            }
            if (!result) {
                return false;
            }
            if (!CallbackContext.Finish()) {
                return false;
            }
            return true;
        } catch (...) {
            TFLEventLog::Error(CurrentExceptionMessage())("content", Content)("code", "cannot_parse_http_reply")("content_type", contentType)("content_type_original", cType ? cType->Value() : "nothing");
            return false;
        }
    }

    bool TParser::ParseXML() {
        NXml::TDocument xmlDoc(TString(Content.data(), Content.size()), NXml::TDocument::String);
        return CallbackContext.ParseXML(xmlDoc);
    }

    bool TParser::ParseJson() {
        NJson::TJsonValue jsonInfo = NJson::ReadJsonFastTree(Content);
        return CallbackContext.ParseJson(jsonInfo);
    }

    bool TParser::ParseText() {
        return CallbackContext.ParseText(Content);
    }

    bool TParser::ParseBinary() {
        return CallbackContext.ParseBinary(Content);
    }

    bool TParser::ParseZip() {
        return CallbackContext.ParseZip(Content);
    }

    bool TContentTypeParser::Parse(const TString& contentType) {
        if (contentType.find("multipart/") != TString::npos) {
            ContentType = EContentType::Multipart;
            const TVector<TString> params = StringSplitter(contentType).SplitBySet("; ").SkipEmpty().ToList<TString>();
            TStringBuf boundary;
            for (auto&& i : params) {
                if (i.StartsWith("boundary=")) {
                    boundary = i;
                    boundary.Skip(9);
                }
            }
            while (boundary.StartsWith('\"')) {
                boundary.Skip(1);
            }
            while (boundary.EndsWith('\"')) {
                boundary.Chop(1);
            }
            Boundary = TString(boundary);
            if (!Boundary) {
                TFLEventLog::Error("incorrect multipart boundary (empty)");
                return false;
            }
        } else if (contentType.find("/xml") != TString::npos) {
            ContentType = EContentType::XML;
        } else if (contentType.find("/json") != TString::npos) {
            ContentType = EContentType::Json;
        } else if (contentType.find("text/") != TString::npos) {
            ContentType = EContentType::Text;
        } else if (contentType.find("/zip") != TString::npos) {
            ContentType = EContentType::Zip;
        } else {
            ContentType = EContentType::Binary;
        }
        return true;
    }

}
