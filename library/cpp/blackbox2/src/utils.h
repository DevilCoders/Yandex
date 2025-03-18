#pragma once

#include <library/cpp/blackbox2/blackbox2.h>

#include <algorithm>
#include <functional>
#include <iostream>
#include <string>

namespace NBlackbox2 {
    const unsigned MAX_REQUEST_LENGTH = 1024;

    TString URLEncode(const TStringBuf str);

    TString URLDecode(const TStringBuf str);

    TString Bin2base64url(const char* buf, size_t len, bool pad = false);
    TString Bin2base64url(const TStringBuf buf, bool pad = false);

    TString Base64url2bin(const char* buf, size_t len);
    TString Base64url2bin(const TStringBuf buf);

    TString HMAC_sha256(const TStringBuf key, const char* data, size_t dataLen);
    TString HMAC_sha256(const TStringBuf key, const TStringBuf data);

    class TRequestBuilder {
        TString Request_;

    public:
        TRequestBuilder(const TStringBuf method);

        void Add(const TStringBuf key, const TStringBuf val);
        void Add(const TStringBuf key, const TVector<TString>& values);
        void Add(const TOption& opt);
        void Add(const TOptions& options);

        operator TString() {
            return Request_;
        }
    };

    // Human-readable output of enumerations
    std::ostream& operator<<(std::ostream& out, TLoginResp::EStatus s);
    std::ostream& operator<<(std::ostream& out, TLoginResp::EAccountStatus s);
    std::ostream& operator<<(std::ostream& out, TLoginResp::EPasswdStatus s);
    std::ostream& operator<<(std::ostream& out, TSessionResp::EStatus s);
    std::ostream& operator<<(std::ostream& out, TAliasList::TItem::EType t);
    std::ostream& operator<<(std::ostream& out, NSessionCodes::ESessionError e);
    std::ostream& operator<<(std::ostream& out, TBruteforcePolicy::EMode m);
    std::ostream& operator<<(std::ostream& out, TSessionKind::EKind k);

    // Human-readable output of list items
    std::ostream& operator<<(std::ostream& out, const TEmailList::TItem& item);
    std::ostream& operator<<(std::ostream& out, const TAliasList::TItem& item);

    std::ostream& operator<<(std::ostream& out, const TEmailList& data);
    std::ostream& operator<<(std::ostream& out, const TAliasList& data);
    std::ostream& operator<<(std::ostream& out, const TDBFields& data);
    std::ostream& operator<<(std::ostream& out, const TAttributes& data);
    std::ostream& operator<<(std::ostream& out, const TOAuthInfo& data);

    // Human-readable output of all known Response information
    std::ostream& operator<<(std::ostream& out, const TResponse* resp);
    std::ostream& operator<<(std::ostream& out, const TBulkResponse* pBulkResp);
    std::ostream& operator<<(std::ostream& out, const TLoginResp* pLogResp);
    std::ostream& operator<<(std::ostream& out, const TSessionResp* pSessResp);
    std::ostream& operator<<(std::ostream& out, const TMultiSessionResp* pMultiSessResp);

    // Human-readable output for response data structures
    std::ostream& operator<<(std::ostream& out, const TPDDUserInfo& info);
    std::ostream& operator<<(std::ostream& out, const TKarmaInfo& info);
    std::ostream& operator<<(std::ostream& out, const TDisplayNameInfo& info);
    std::ostream& operator<<(std::ostream& out, const TAuthInfo& info);
}

namespace NIdn {
    // Note: all encode/decode functions return reference to originalTString if no transformation needed

    // encode entireTString to punycode
    const TString& Encode(const TString& str, TString& converted);

    // decode entireTString as punycode
    const TString& Decode(const TString& str, TString& converted);

    // encode address to punycode
    const TString& EncodeAddr(const TString& addr, TString& converted);

    // decode address as punycode
    const TString& DecodeAddr(const TString& addr, TString& converted);
}
