#pragma once

#include <library/cpp/http/server/response.h>


class IOutputStream;

namespace NAntiRobot {

const TString FORWARD_TO_USER_HEADER = "X-ForwardToUser-Y";

class TResponse {
public:
    static TResponse ToUser(HttpCodes code, bool withExternalRequests = false);
    static TResponse ToBalancer(HttpCodes code, bool withExternalRequests = false);

    TResponse(const TResponse&) = default;
    TResponse(TResponse&&) = default;

    /// Calls ToUser() to create a TResponse instance
    static TResponse Redirect(const TStringBuf& location, bool withExternalRequests = false);

    template<typename ValueType>
    TResponse& AddHeader(const TString& name, const ValueType& value) {
        Resp.AddHeader(name, value);
        return *this;
    }

    TResponse& AddHeader(const THttpInputHeader& header) {
        Resp.AddHeader(header);
        return *this;
    }

    TResponse& AddHeaders(const THttpHeaders& headers) {
        Resp.AddMultipleHeaders(headers);
        return *this;
    }

    const THttpHeaders& GetHeaders() const {
        return Resp.GetHeaders();
    }

    TResponse& SetContent(const TString& content) {
        Resp.SetContent(content);
        return *this;
    }

    TResponse& SetContent(const TString& content, const TStringBuf& contentType) {
        Resp.SetContent(content, contentType);
        return *this;
    }

    TResponse& DisableCompressionHeader() {
        DisableCompressionHeaderFlag = true;
        return *this;
    }

    bool HasDisableCompressionHeader() const {
        return DisableCompressionHeaderFlag;
    }

    bool HadExternalRequests() const {
        return WithExternalRequests;
    }

    bool IsForwardedToUser() const {
        return ForwardToUser;
    }

    bool IsForwardedToBalancer() const {
        return !ForwardToUser;
    }

    void SetExternalRequestsFlag(bool withExternalRequests) {
        WithExternalRequests = withExternalRequests;
    }

    HttpCodes GetHttpCode() const {
        return Resp.HttpCode();
    }

private:
    TResponse(HttpCodes code, bool forwardToUser, bool withExternalRequests);
    friend IOutputStream& operator << (IOutputStream& os, const TResponse& response);

private:
    THttpResponse Resp;
    bool WithExternalRequests = false;
    bool ForwardToUser = false;
    bool DisableCompressionHeaderFlag = false;
};

}
