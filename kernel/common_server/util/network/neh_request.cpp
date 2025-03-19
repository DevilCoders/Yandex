#include "neh_request.h"

#include <library/cpp/logger/global/global.h>
#include <library/cpp/neh/neh.h>
#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/string_utils/url/url.h>
#include <kernel/common_server/util/json_processing.h>
#include <library/cpp/string_utils/base64/base64.h>

namespace NNeh {

    namespace {
        TSet<TString> SecretCgiParameters = {"user_ticket"};
    }

    NNeh::THttpRequest& THttpRequest::SetTraceHeader(const TString& value) {
        TraceHeader = value;
        if (!!TraceHeader) {
            AddHeader("X-YaTraceId", TraceHeader);
        }
        return *this;
    }

    void THttpRequest::RegisterHiddenCgi(const TString& key) {
        SecretCgiParameters.emplace(key);
    }

    TMessage THttpRequestBuilder::MakeNehMessage(const THttpRequest& request) const {
        TStringStream ssAddr;
        ssAddr << TString(IsHttps ? "fulls" : "full");
        ssAddr << "://";
        if (!!Cert && !!CertKey) {
            ssAddr << "cert=" << Cert << ";" << "key=" << CertKey << "@";
        }
        ssAddr << Host + ":" + ToString(Port);
        TMessage result = TMessage::FromString(ssAddr.Str());
        if (request.GetRequestType() == "GET") {
            CHECK_WITH_LOG(request.GetPostData().Empty());
        }
        TStringStream ss;
        TStringBuf CrLf = "\r\n";

        TStringBuf host = Host;

        const TString& requestStr = request.GetRequest();
        ss << request.GetRequestType() << " "sv;
        if (request.GetTargetUrl()) {
            host = GetHostAndPort(request.GetTargetUrl());
            ss << request.GetTargetUrl();
        }
        if (request.GetRequestType() != "CONNECT") {
            if (!requestStr.StartsWith("/")) {
                ss << "/"sv;
            }
            ss << requestStr;
        }
        ss << " HTTP/1.1"sv << CrLf;
        ss << "Host: "sv << host;
        for (auto&& header : CommonHeaders) {
            ss << CrLf << header.first << ": "sv << header.second;
        }
        for (auto&& header : request.GetHeaders()) {
            ss << CrLf << header.first << ": "sv << header.second;
        }

        if (!request.GetPostData().Empty()) {
            ss << CrLf << "Content-Length: "sv << request.GetPostData().Size();
            ss << CrLf << CrLf;
            ss.Write(request.GetPostData().AsStringBuf());
        } else {
            ss << CrLf << CrLf;
        }
        result.Data = ss.Str();
        return result;
    }

    TString THttpRequestBuilder::GetScript() const {
        return (IsHttps ? "fulls://" : "full://") + Host + ":" + ::ToString(Port);
    }

    THttpRequest& THttpRequest::SetConfigMeta(const NSimpleMeta::TConfig& config) {
        ConfigMeta = config;
        return *this;
    }

    TString THttpRequest::GetDebugRequest() const {
        TStringStream ss = "REQUEST " + RequestType + ":\n";
        ss << Uri << (CgiData.size() ? "?" + GetCgiData() : "") << Endl;
        ss << TStringBuf(PostData.AsCharPtr(), PostData.Length()) << Endl;
        for (auto&& i : Headers) {
            ss << i.first << ": " << i.second << Endl;
        }
        return ss.Str();
    }

    TString THttpRequest::GetCgiData(const bool forLogging, const TSet<TString>& hiddenCgiParameters) const {
        TString result;
        for (auto&& i : CgiData) {
            if (!!result) {
                result += "&";
            }
            if (!forLogging || (!SecretCgiParameters.contains(i.first) && !hiddenCgiParameters.contains(i.first))) {
                result += i.first + "=" + i.second;
            } else {
                result += i.first + "=" + MD5::Calc(i.second);
            }
        }
        return result;
    }

    THttpRequest& THttpRequest::AddCgiData(const TString& data) {
        TVector<TString> cgi = StringSplitter(data).Split('&').SkipEmpty().ToList<TString>();
        for (auto&& i : cgi) {
            TStringBuf sb(i.data(), i.size());
            TStringBuf l;
            TStringBuf r;
            if (!sb.TrySplit('=', l, r)) {
                continue;
            }
            CgiData.emplace(l, r);
        }
        return *this;
    }

    NJson::TJsonValue THttpRequest::SerializeToJson() const {
        NJson::TJsonValue result;
        result.InsertValue("uri", Uri);
        result.InsertValue("content", Base64Encode(PostData.AsStringBuf()));
        result.InsertValue("content_b64", Base64Encode(PostData.AsStringBuf()));
        result.InsertValue("request_type", RequestType);
        result.InsertValue("target_url", TargetUrl);
        TJsonProcessor::WriteMap(result, "headers", Headers);
        TJsonProcessor::WriteMap(result, "cgi_data", CgiData);
        return result;
    }

    bool THttpRequest::DeserializeFromJson(const NJson::TJsonValue& jsonData) {
        NJson::TJsonValue result;
        if (!TJsonProcessor::Read(jsonData, "uri", Uri, true)) {
            return false;
        }
        if (jsonData.Has("content_b64")) {
            TString content;
            if (!TJsonProcessor::Read(jsonData, "content_b64", content)) {
                return false;
            } else if (content) {
                try {
                    PostData = TBlob::FromString(Base64Decode(content));
                } catch (...) {
                    TFLEventLog::Error("cannot parse base64 for http_request");
                    return false;
                }
            }
        } else {
            TString content;
            if (!TJsonProcessor::Read(jsonData, "content", content)) {
                return false;
            } else if (content) {
                try {
                    PostData = TBlob::FromString(Base64Decode(content));
                } catch (...) {
                    PostData = TBlob::FromString(std::move(content));
                }
            }
        }
        if (!TJsonProcessor::Read(jsonData, "request_type", RequestType, true)) {
            return false;
        }
        if (!TJsonProcessor::Read(jsonData, "target_url", TargetUrl, true)) {
            return false;
        }
        if (!TJsonProcessor::ReadMap(jsonData, "headers", Headers)) {
            return false;
        }
        if (!TJsonProcessor::ReadMap(jsonData, "cgi_data", CgiData)) {
            return false;
        }
        return true;
    }

}
