#pragma once

#include "http_request_enum.h"

#include <library/cpp/http/io/headers.h>

#include <util/datetime/base.h>
#include <util/generic/map.h>
#include <util/generic/string.h>
#include <util/network/socket.h>
#include <kernel/common_server/util/accessor.h>

namespace NJson {
    class TJsonValue;
}

namespace NUtil {
    class THttpReply {
    private:
        bool IsConnected_ = false;
        bool IsCorrectReply_ = false;
        ui32 Code_ = 0;
        TString ErrorMessage_;
        TString Content_;
        ui32 Flags = 0;
        THttpHeaders Headers;
        CSA_MAYBE(THttpReply, TInstant, StartInstant);
    public:
        using EFlags = EHttpReplyFlags;

    public:
        TString GetDebugReply() const;

        TVector<EFlags> GetFlags() const;

        THttpReply& AddFlag(const EFlags flag) {
            Flags |= (ui32)flag;
            return *this;
        }

        bool HasReply() const {
            return !IsTimeout() && !IsUnknownProblem();
        }

        bool IsTimeout() const {
            return Flags & (ui32)EFlags::Timeout;
        }

        bool IsUnknownProblem() const {
            return Flags & (ui32)EFlags::Unknown;
        }

        bool IsErrorInResponse() const {
            return Flags & (ui32)EFlags::ErrorInResponse;
        }

        NJson::TJsonValue Serialize() const;

        bool IsSuccessReply() const {
            return Code_ == 200 || Code_ == 202;
        }

        bool IsUserError() const {
            return Code_ >= 400 && Code_ < 500;
        }

        bool IsServerError() const {
            return Code_ >= 500;
        }

        void SetIsConnected(bool value) {
            IsConnected_ = value;
        }

        bool IsConnected() const {
            return IsConnected_;
        }

        void SetIsCorrectReply(bool value) {
            IsCorrectReply_ = value;
        }

        bool IsCorrectReply() const {
            return IsCorrectReply_;
        }

        ui32 Code() const {
            return Code_;
        }

        THttpReply& SetCode(ui32 value) {
            Code_ = value;
            return *this;
        }

        const TString& Content() const {
            return Content_;
        }

        void SetContent(const TString& value) {
            Content_ = value;
        }

        void SetErrorMessage(const TString& value) {
            ErrorMessage_ = value;
        }

        const TString& ErrorMessage() const {
            return ErrorMessage_;
        }

        void SetHeaders(const THttpHeaders& headers) {
            Headers = headers;
        }

        const THttpHeaders& GetHeaders() const {
            return Headers;
        }
    };

    class THttpRequest {
    private:
        TString Command;
        TString PostData;
        bool IsHttps = false;
        TDuration Timeout = TDuration::MilliSeconds(100);
        ui32 SendAttemptionsMax = 5;
        TDuration SleepingPause = TDuration::MilliSeconds(100);
        TMap<TString, TString> AdditionHeaders;
        TString ContentType = "application/octet-stream";

    public:
        THttpRequest(const TString& command)
            : Command(command)
        {
        }

        THttpRequest(const TString& command, const TString& postData) {
            Command = command;
            PostData = postData;
        }

        THttpRequest& SetHeader(const TString& header, const TString& value) {
            AdditionHeaders[header] = value;
            return *this;
        }

        THttpRequest& SetIsHttps(bool value = true) {
            IsHttps = value;
            return *this;
        }

        THttpRequest& SetContentType(const TString& value) {
            ContentType = value;
            return *this;
        }

        THttpRequest& SetCommand(const TString& value) {
            Command = value;
            return *this;
        }

        THttpRequest& SetPostData(const TString& value) {
            PostData = value;
            return *this;
        }

        THttpRequest& SetSleepingPause(ui32 pauseMs) {
            SleepingPause = TDuration::MilliSeconds(pauseMs);
            return *this;
        }

        THttpRequest& SetTimeout(ui32 timeoutMs) {
            Timeout = TDuration::MilliSeconds(timeoutMs);
            return *this;
        }

        THttpRequest& SetAttemptionsMax(ui32 sendAttemptionsMax) {
            SendAttemptionsMax = sendAttemptionsMax;
            return *this;
        }

        THttpReply Execute(const TString& host, ui32 port);
    };

    bool SendRequest(TSocket& socket, const TString& data, THttpReply& result, bool isHttps, bool readContent);
    bool SendRequest(const TString& host, ui16 port, const TString& data, const TDuration& timeout, THttpReply& result, bool isHttps);

}
