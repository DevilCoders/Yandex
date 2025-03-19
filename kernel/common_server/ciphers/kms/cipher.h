#pragma once

#include <kernel/common_server/ciphers/external.h>

namespace NCS {
    class TKmsCipherConfig: public IExternalCipherConfig {
    private:
        using TBase = IExternalCipherConfig;

        CSA_READONLY_DEF(TString, KmsKeyId);
        CSA_READONLY_DEF(TString, PrivateKey);
        CSA_READONLY_DEF(TString, PublicKey);
        CSA_READONLY_DEF(TString, ServiceAccId);
        CSA_READONLY_DEF(TString, ServiceAccKeyId);
        CSA_READONLY_DEF(TString, AuthApiName);
        CSA_READONLY_DEF(TString, KeyApiName);

        static TFactory::TRegistrator<TKmsCipherConfig> Registrator;

    protected:
        virtual IAbstractCipher::TPtr DoConstruct(const IBaseServer* server) const override;
        virtual void DoInit(const TYandexConfig::Section* section) final;
        virtual void DoToString(IOutputStream& os) const final;

    public:
        using TBase::TBase;
    };

    class TKmsEncodeRequest: public NExternalAPI::IHttpRequestWithJsonReport {
    private:
        CSA_DEFAULT(TKmsEncodeRequest, TString, Token);
        CSA_DEFAULT(TKmsEncodeRequest, TString, KeyId);
        CSA_DEFAULT(TKmsEncodeRequest, TString, Plain);

    public:
        virtual bool BuildHttpRequest(NNeh::THttpRequest& request) const final;

        class TResponse: public TJsonResponse {
        private:
            CSA_DEFAULT(TResponse, TString, Encoded);

        protected:
            virtual bool DoParseJsonReply(const NJson::TJsonValue& json) override;
        };
        TKmsEncodeRequest(const TString& token, const TString& id, const TString& plain)
            : Token(token)
            , KeyId(id)
            , Plain(plain)
        {
        }
    };
    class TKmsDecodeRequest: public NExternalAPI::IHttpRequestWithJsonReport {
    private:
        CSA_DEFAULT(TKmsDecodeRequest, TString, Token);
        CSA_DEFAULT(TKmsDecodeRequest, TString, KeyId);
        CSA_DEFAULT(TKmsDecodeRequest, TString, Encoded);

    public:
        virtual bool BuildHttpRequest(NNeh::THttpRequest& request) const final;

        class TResponse: public TJsonResponse {
        private:
            CSA_DEFAULT(TResponse, TString, Plain);
            CSA_DEFAULT(TResponse, TString, VersionId);

        protected:
            virtual bool DoParseJsonReply(const NJson::TJsonValue& json) override;
        };
        TKmsDecodeRequest(const TString& token, const TString& id, const TString& encoded)
            : Token(token)
            , KeyId(id)
            , Encoded(encoded)
        {
        }
    };

    class TKmsCipher: public IExternalCipher<TKmsEncodeRequest, TKmsDecodeRequest, TKmsCipherConfig> {
    private:
        using TBase = IExternalCipher<TKmsEncodeRequest, TKmsDecodeRequest, TKmsCipherConfig>;

        TString RequestAuthToken() const;

        mutable TInstant ExpiredDeadline = TInstant::Zero();
        mutable TString BearerToken;

        TThreadPool AuthTokenRefreshPool;
        mutable TRWMutex AuthTokenMutex;
        class TAuthTokenRefreshAgent: public IObjectInQueue {
        private:
            const TKmsCipher* Owner;

        public:
            TAuthTokenRefreshAgent(const TKmsCipher* owner)
                : Owner(owner)
            {
            }

            virtual void Process(void* /*threadSpecificResource*/) override;
        };

        mutable TString PrimaryVersion;
        TThreadPool PrimaryVersionRefreshPool;
        mutable TRWMutex PrimaryVersionMutex;
        class TPrimaryVersionRefreshAgent: public IObjectInQueue {
        private:
            const TKmsCipher* Owner;

        public:
            TPrimaryVersionRefreshAgent(const TKmsCipher* owner)
                : Owner(owner)
            {
            }

            virtual void Process(void* /*threadSpecificResource*/) override;
        };

        const TString& GetBearerToken() const {
            TReadGuard wg(AuthTokenMutex);
            return BearerToken;
        }

        const TString& GetPrimaryVersion() const {
            TReadGuard wg(PrimaryVersionMutex);
            return PrimaryVersion;
        }

        void TryUpdateToken(int num) const;
        void UpdatePrimaryVersion() const;

    protected:
        class TReEncryptRequest: public NExternalAPI::IHttpRequestWithJsonReport {
        private:
            CSA_DEFAULT(TReEncryptRequest, TString, Token);
            CSA_DEFAULT(TReEncryptRequest, TString, KeyId);
            CSA_DEFAULT(TReEncryptRequest, TString, Encrypted);

        public:
            virtual bool BuildHttpRequest(NNeh::THttpRequest& request) const final;

            class TResponse: public TJsonResponse {
            private:
                CSA_DEFAULT(TResponse, TString, Encrypted);

            protected:
                virtual bool DoParseJsonReply(const NJson::TJsonValue& json) override;
            };
            TReEncryptRequest(const TString& token, const TString& id, const TString& encrypted)
                : Token(token)
                , KeyId(id)
                , Encrypted(encrypted)
            {
            }
        };

        class TGetRequest: public NExternalAPI::IHttpRequestWithJsonReport {
        private:
            CSA_DEFAULT(TGetRequest, TString, Token);
            CSA_DEFAULT(TGetRequest, TString, KeyId);

        public:
            virtual bool BuildHttpRequest(NNeh::THttpRequest& request) const final;

            class TResponse: public TJsonResponse {
            private:
                CSA_DEFAULT(TResponse, TString, PrimaryVersionId);

            protected:
                virtual bool DoParseJsonReply(const NJson::TJsonValue& json) override;
            };
            TGetRequest(const TString& token, const TString& id)
                : Token(token)
                , KeyId(id)
            {
            }
        };

        virtual IAbstractCipher::TPtr DoCreateNewVersion() const override {
            return MakeAtomicShared<TKmsCipher>(Server, Config);
        }

        virtual bool ReencryptIfNeeded(TString& encrypted, bool& isReencrypted) const override;

        virtual TKmsEncodeRequest::TResponse SendEncodeRequest(const NExternalAPI::TSender::TPtr api, const TString& plain) const override {
            return api->SendRequest<TKmsEncodeRequest>(GetBearerToken(), Config.GetKmsKeyId(), plain);
        }

        virtual TKmsDecodeRequest::TResponse SendDecodeRequest(const NExternalAPI::TSender::TPtr api, const TString& encrypted) const override {
            return api->SendRequest<TKmsDecodeRequest>(GetBearerToken(), Config.GetKmsKeyId(), encrypted);
        }

    public:
        TKmsCipher(const IBaseServer& server, const TKmsCipherConfig& config);
        ~TKmsCipher();

        virtual bool NeedRotate() const override {
            return false;
        }

        virtual TString GetVersion() const override {
            return GetPrimaryVersion();
        }
    };
}
