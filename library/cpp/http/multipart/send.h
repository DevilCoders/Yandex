#pragma once

#include <library/cpp/http/io/headers.h>

#include <util/stream/str.h>

extern const TString DefaultBoundary;

//TContainer - TVector<std::pair<TString, TString> >, TMap<TStringBuf, TStringBuf>, THashMap<const char*, TString> etc.
template <class TContainer>
TString BuildMultipartHttpRequest(const TContainer& parts, TStringBuf uri, TStringBuf host, TStringBuf boundary = DefaultBoundary, const THttpHeaders& headers = THttpHeaders()) {
    TStringStream body;
    for (const auto& part : parts) {
        body << "\r\n--" << boundary << "\r\n";
        body << "Content-Disposition: form-data; name=\"" << part.first << "\"\r\n";
        body << "Content-Length: " << part.second.size() << "\r\n";
        body << "\r\n"
             << part.second;
    }
    body << "\r\n--" << boundary << "--\r\n";

    TStringStream data;
    data << "POST " << uri << " HTTP/1.1\r\n";
    data << "Host: " << host << "\r\n";
    for (const THttpInputHeader& header : headers) {
        data << header.Name() << ": " << header.Value() << "\r\n";
    }
    data << "Content-Type: multipart/form-data; boundary=" << boundary << "\r\n";
    data << "Content-Length: " << body.Size() << "\r\n";
    data << "\r\n"
         << body.Str();
    return data.Str();
}
