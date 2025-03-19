#pragma once

#include <kernel/common_server/library/metasearch/simple/config.h>
#include <kernel/common_server/util/accessor.h>
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/string_utils/quote/quote.h>

#include <util/generic/set.h>
#include <util/generic/map.h>
#include <util/generic/string.h>
#include <util/memory/blob.h>
#include <util/string/cast.h>

namespace NNeh {
    struct TMessage;

    class THttpRequest {
    private:
        TString Uri;
        TMultiMap<TString, TString> CgiData;
        TMap<TString, TString> Headers;
        TString RequestType = "GET";
        TBlob PostData;
        TMaybe<NSimpleMeta::TConfig> ConfigMeta;
        CSA_DEFAULT(THttpRequest, TString, TargetUrl);
        CSA_READONLY_DEF(TString, TraceHeader);
    public:
        THttpRequest& SetTraceHeader(const TString& value);

        const NSimpleMeta::TConfig* GetConfigMeta() const {
            return ConfigMeta ? &*ConfigMeta : nullptr;
        }

        static void RegisterHiddenCgi(const TString& key);

        THttpRequest& SetConfigMeta(const NSimpleMeta::TConfig& config);

        TString GetDebugRequest() const;

        THttpRequest& SetContentType(const TString& value) {
            return AddHeader("Content-Type", value);
        }

        THttpRequest& SetAuthorization(const TString& value) {
            return AddHeader("Authorization", value);
        }

        THttpRequest& SetOAuthToken(const TString& token) {
            return AddHeader("Authorization", "OAuth " + token);
        }

        THttpRequest& SetAccept(const TString& value) {
            static const TString name = "Accept";
            return AddHeader(name, value);
        }

        TString GetRequest(const bool forLogging = false, const TSet<TString>& hiddenCgiParameters = Default<TSet<TString>>()) const {
            return Uri + (CgiData.size() ? "?" + GetCgiData(forLogging, hiddenCgiParameters) : "");
        }

        const TString& GetUri() const {
            return Uri;
        }

        bool HasCgi(const TString& cgiName) const {
            return CgiData.contains(cgiName);
        }
        TString GetCgiData(const bool forLogging = false, const TSet<TString>& hiddenCgiParameters = Default<TSet<TString>>()) const;

        const TMap<TString, TString>& GetHeaders() const {
            return Headers;
        }

        const TString& GetRequestType() const {
            return RequestType;
        }

        const TBlob& GetPostData() const {
            return PostData;
        }

        THttpRequest& SetPostData(const TBlob& value, const TString& reqType = "POST") {
            PostData = value;
            return SetRequestType(reqType);
        }

        THttpRequest& SetPostData(const NJson::TJsonValue& value, const TString& reqType = "POST") {
            return SetPostData(value.GetStringRobust(), reqType).AddHeader("Content-Type", "application/json");
        }

        THttpRequest& SetPostData(const TString& value, const TString& reqType = "POST") {
            return SetPostData(TBlob::FromString(value), reqType);
        }

        THttpRequest& AddHeader(const TString& name, const TString& value) {
            Headers[name] = value;
            return *this;
        }

        template <class T>
        THttpRequest& AddHeader(const TString& name, const T& value) {
            Headers[name] = ::ToString(value);
            return *this;
        }

        THttpRequest& SetHeaders(const TMap<TString, TString>& headers) {
            Headers = headers;
            return *this;
        }

        THttpRequest& SetUri(const TString& uri) {
            Uri = uri;
            return *this;
        }

        THttpRequest& SetCgiData(const TString& data) {
            CgiData.clear();
            return AddCgiData(data);
        }

        THttpRequest& AddCgiData(const TString& data);

        NJson::TJsonValue SerializeToJson() const;
        bool DeserializeFromJson(const NJson::TJsonValue& jsonData);

        class TGuard: public TNonCopyable {
        protected:
            class TCgiTraits {
            private:
                CSA_FLAG(TCgiTraits, IgnoreEmpty, false);
                CSA_FLAG(TCgiTraits, Encode, false);
            };

            TCgiTraits Traits;
        private:
            THttpRequest& Request;
            TMap<TString, TString> Args;
        public:
            explicit TGuard(THttpRequest& request)
                : Request(request)
            {
            }

            template<typename T>
            TGuard(THttpRequest& request, const TString& key, const T& value)
                : Request(request)
            {
                Args.emplace(key, ::ToString(value));
            }

            template <typename T>
            TGuard& Add(const TString& key, const T& value) {
                Args.emplace(key, ::ToString(value));
                return *this;
            }

            ~TGuard() {
                for (auto&& [key, value] : Args) {
                    if (Traits.IsIgnoreEmpty() && value.empty()) {
                        continue;
                    }
                    if (Traits.IsEncode()) {
                        Request.CgiData.emplace(key, CGIEscapeRet(value));
                    } else {
                        Request.CgiData.emplace(key, std::move(value));
                    }
                }
            }
        };

        class TCgiInserterGuard: public TGuard {
            using TBase = TGuard;
        public:
            using TBase::TBase;

            TCgiInserterGuard& Encode() {
                Traits.SetEncode(true);
                return *this;
            }

            TCgiInserterGuard& IgnoreEmpty() {
                Traits.SetIgnoreEmpty(true);
                return *this;
            }
        };

        template <typename T>
        TCgiInserterGuard AddCgiData(const TString& key, const T& value) {
            return TCgiInserterGuard(*this, key, value);
        }

        TCgiInserterGuard CgiInserter() {
            return TCgiInserterGuard(*this);
        }

        THttpRequest& SetRequestType(const TString& value) {
            RequestType = value;
            return *this;
        }
    };

    class THttpRequestBuilder {
    private:
        TString Host;
        ui16 Port;
        bool IsHttps = false;
        TString Cert;
        TString CertKey;
        TMap<TString, TString> CommonHeaders;

    public:
        THttpRequestBuilder(const TString& host, const ui16 port, const bool isHttps = false, const TString& cert = "", const TString& certKey = "")
            : Host(host)
            , Port(port)
            , IsHttps(isHttps)
            , Cert(cert)
            , CertKey(certKey)
        {
        }

        THttpRequestBuilder& AddHeader(const TString& name, const TString& value) {
            CommonHeaders[name] = value;
            return *this;
        }

        TString GetScript() const;

        NNeh::TMessage MakeNehMessage(const THttpRequest& request) const;
    };
}
