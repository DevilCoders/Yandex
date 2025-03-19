#include "http_request.h"

#include <library/cpp/logger/global/global.h>
#include <library/cpp/openssl/io/stream.h>

#include <library/cpp/http/io/stream.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>

#include <util/datetime/base.h>
#include <util/generic/serialized_enum.h>
#include <library/cpp/string_utils/base64/base64.h>

namespace NUtil {

    bool SendRequest(const TString& host, ui16 port, const TString& data, const TDuration& timeout, THttpReply& result, bool isHttps) {
        result.SetIsConnected(false);
        result.SetIsCorrectReply(false);
        result.SetErrorMessage("");
        try {
            TSocket s(TNetworkAddress(host, port), timeout);
            s.SetSocketTimeout(1 + timeout.Seconds());
            return SendRequest(s, data, result, isHttps, true);
        } catch (...) {
            result.SetErrorMessage(CurrentExceptionMessage());
            return false;
        }
    }

    bool SendRequest(TSocket & s, const TString& data, THttpReply& result, bool isHttps, bool readContent) {
        try {
            TSocketOutput so(s);
            TSocketInput si(s);
            THolder<TOpenSslClientIO> ssl;
            THolder<THttpInput> hi;
            if (isHttps) {
                ssl = MakeHolder<TOpenSslClientIO>(&si, &so);
                THttpOutput ho(ssl.Get());
                ho << data;
                ho.Finish();
                hi = MakeHolder<THttpInput>(ssl.Get());
            } else {
                so << data;
                so.Flush();
                hi = MakeHolder<THttpInput>(&si);
            }
            result.SetCode(ParseHttpRetCode(hi->FirstLine()));
            ui64 contentLength;
            if (hi->GetContentLength(contentLength)) {
                TString buffer;
                buffer.resize(contentLength);
                hi->Load(buffer.begin(), contentLength);
                result.SetContent(buffer);
            } else if (readContent) {
                result.SetContent(hi->ReadAll());
            }
            result.SetHeaders(hi->Headers());
            result.SetIsConnected(true);
            result.SetIsCorrectReply(true);
            return true;
        } catch (...) {
            result.SetErrorMessage(CurrentExceptionMessage());
            return false;
        }
    }

    TString THttpReply::GetDebugReply() const {
        TStringStream ss = "REPLY " + ::ToString((i32)Code_) + "\n";
        if (!!ErrorMessage_) {
            ss << "ERROR: " << ErrorMessage_ << Endl;
        }
        if (!!Content_) {
            NJson::TJsonValue replyJson;
            try {
                if (NJson::ReadJsonFastTree(Content_, &replyJson)) {
                    ss << "CONTENT: " << Endl << NJson::WriteJson(replyJson) << Endl;
                } else {
                    ss << "CONTENT: " << Endl << Content_ << Endl;
                }
            } catch (...) {
                ss << "CONTENT: " << Endl << Content_ << Endl;
            }
        }
        return ss.Str();
    }

    TVector<THttpReply::EFlags> THttpReply::GetFlags() const {
        TVector<THttpReply::EFlags> flags;
        for (const auto& x : GetEnumAllValues<THttpReply::EFlags>()) {
            if (static_cast<ui32>(x) & Flags) {
                flags.emplace_back(x);
            }
        }
        return flags;
    }

    NJson::TJsonValue THttpReply::Serialize() const {
        NJson::TJsonValue result(NJson::JSON_MAP);
        result.InsertValue("code", Code_);
        if (!!ErrorMessage_)
            result.InsertValue("error", ErrorMessage_);
        result.InsertValue("is_connected", IsConnected_);
        result.InsertValue("is_correct_reply", IsCorrectReply_);
        NJson::TJsonValue contentJson;
        TStringInput si(Content_);
        if (NJson::ReadJsonTree(&si, &contentJson)) {
            result.InsertValue("content_json", contentJson);
        } else {
            result.InsertValue("content_b64", Base64Encode(Content_));
        }
        {
            NJson::TJsonValue& jsonArr = result.InsertValue("headers", NJson::JSON_ARRAY);
            for (auto&& header : Headers) {
                NJson::TJsonValue& jsonItem = jsonArr.AppendValue(NJson::JSON_MAP);
                jsonItem.InsertValue("key", header.Name());
                jsonItem.InsertValue("value", header.Value());
            }
        }
        return result;
    }

    THttpReply THttpRequest::Execute(const TString& host, ui32 port) {
        DEBUG_LOG << "actor=http_request;receiver=" << host << ":" << port << ";command=" << Command << ";status=starting;" << Endl;
        TStringStream ss;

        if (!PostData) {
            ss << "GET /" << Command << " HTTP/1.1\r\n";
            ss << "Host:" << host << "\r\n";
            ss << "Content-Type:" << ContentType << "\r\n";

            for (const auto& header : AdditionHeaders) {
                ss << header.first << ":" << header.second << "\r\n";
                INFO_LOG << "addition header: " << header.first << " : " << header.second << Endl;
            }

            ss << "\r\n";
        } else {
            ss << "POST /" << Command << " HTTP/1.1\r\n";
            ss << "Content-Length:" << PostData.size() << "\r\n";
            ss << "Content-Type:" << ContentType << "\r\n";

            for (const auto& header : AdditionHeaders) {
                ss << header.first << ":" << header.second << "\r\n";
                DEBUG_LOG << "additional header: " << header.first << " : " << header.second << Endl;
            }

            ss << "Host:" << host << "\r\n";
            ss << "\r\n";
            ss << PostData;
        }
        THttpReply result;
        for (ui32 i = 0; i < SendAttemptionsMax; ++i) {
            if (SendRequest(host, port, ss.Str(), Timeout, result, IsHttps)) {
                DEBUG_LOG << "actor=controller_agent;receiver=" << host << ":" << port << ";command=" << Command << ";status=finished;code=" << result.Code() << Endl;
                break;
            } else {
                ERROR_LOG << "actor=controller_agent;receiver=" << host << ":" << port << ";command=" << Command << ";status=failed;msg=" << result.ErrorMessage() << ";attemption=" << i << Endl;
                Sleep(SleepingPause);
            }
        }
        return result;
    }
}
