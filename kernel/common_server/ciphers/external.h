#pragma once

#include <kernel/common_server/ciphers/abstract.h>
#include <kernel/common_server/ciphers/config.h>
#include <kernel/common_server/library/tvm_services/abstract/abstract.h>
#include <kernel/common_server/abstract/frontend.h>

namespace NCS {

    class IExternalCipherConfig: public ICipherConfig {
    private:
        using TBase = ICipherConfig;
        CSA_PROTECTED_DEF(IExternalCipherConfig, TString, ApiName);

    protected:
        virtual void DoInit(const TYandexConfig::Section* section) override {
            ApiName = section->GetDirectives().Value("ApiName", ApiName);
        }
        virtual void DoToString(IOutputStream& os) const override {
            os << "ApiName: " << ApiName << Endl;
        }

    public:
        using TBase::TBase;
    };

    class TExternalCipherConfig: public IExternalCipherConfig {
    private:
        using TBase = IExternalCipherConfig;

        static TFactory::TRegistrator<TExternalCipherConfig> Registrator;

        CSA_PROTECTED_DEF(IExternalCipherConfig, TString, RequestEncodeField);
        CSA_PROTECTED_DEF(IExternalCipherConfig, TString, RequestPlainField);
        CSA_PROTECTED_DEF(IExternalCipherConfig, TString, ReplyEncodeField);
        CSA_PROTECTED_DEF(IExternalCipherConfig, TString, ReplyPlainField);
        CSA_PROTECTED_DEF(IExternalCipherConfig, TString, EncodeUri);
        CSA_PROTECTED_DEF(IExternalCipherConfig, TString, DecodeUri);

    protected:
        virtual IAbstractCipher::TPtr DoConstruct(const IBaseServer* server) const override;
        virtual void DoInit(const TYandexConfig::Section* section) override;
        virtual void DoToString(IOutputStream& os) const override;

    public:
        using TBase::TBase;
    };

    class TEncodeRequest: public NExternalAPI::IHttpRequestWithJsonReport {
    private:
        CSA_DEFAULT(TEncodeRequest, TString, Plain);
        CSA_DEFAULT(TEncodeRequest, TString, RequestPlainField);
        CSA_DEFAULT(TEncodeRequest, TString, ReplyEncodeField);
        CSA_DEFAULT(TEncodeRequest, TString, EncodeUri);

    public:
        virtual bool BuildHttpRequest(NNeh::THttpRequest& request) const override;

        class TResponse: public TJsonResponse {
        private:
            CSA_DEFAULT(TResponse, TString, Encoded);
            CSA_DEFAULT(TResponse, TString, EncodeField);

        protected:
            virtual bool DoParseJsonReply(const NJson::TJsonValue& json) override;

        public:
            virtual void ConfigureFromRequest(const IServiceApiHttpRequest* request) override;
        };
        TEncodeRequest(const TString& plain, const TExternalCipherConfig& config)
            : Plain(plain)
            , RequestPlainField(config.GetRequestPlainField())
            , ReplyEncodeField(config.GetReplyEncodeField())
            , EncodeUri(config.GetEncodeUri())
        {
        }
    };

    class TDecodeRequest: public NExternalAPI::IHttpRequestWithJsonReport {
    private:
        CSA_DEFAULT(TDecodeRequest, TString, Encoded);
        CSA_DEFAULT(TDecodeRequest, TString, ReplyPlainField);
        CSA_DEFAULT(TDecodeRequest, TString, RequestEncodeField);
        CSA_DEFAULT(TDecodeRequest, TString, DecodeUri);

    public:
        virtual bool BuildHttpRequest(NNeh::THttpRequest& request) const override;

        class TResponse: public TJsonResponse {
        private:
            CSA_DEFAULT(TResponse, TString, Plain);
            CSA_DEFAULT(TResponse, TString, PlainField);

        protected:
            virtual bool DoParseJsonReply(const NJson::TJsonValue& json) override;

        public:
            virtual void ConfigureFromRequest(const IServiceApiHttpRequest* request) override;
        };
        TDecodeRequest(const TString& encoded, const TExternalCipherConfig& config)
            : Encoded(encoded)
            , ReplyPlainField(config.GetReplyPlainField())
            , RequestEncodeField(config.GetRequestEncodeField())
            , DecodeUri(config.GetDecodeUri())
        {
        }
    };

    template <class TEncodeRequest, class TDecodeRequest, class TConfig>
    class IExternalCipher: public IAbstractCipher {
    private:
        using TBase = IAbstractCipher;

    protected:
        const IBaseServer& Server;
        const TConfig Config;

        virtual typename TEncodeRequest::TResponse SendEncodeRequest(const NExternalAPI::TSender::TPtr api, const TString& plain) const = 0;
        virtual typename TDecodeRequest::TResponse SendDecodeRequest(const NExternalAPI::TSender::TPtr api, const TString& plain) const = 0;

    public:
        virtual bool Encrypt(const TString& plain, TString& encoded) const final {
            const auto api = Server.GetSenderPtr(Config.GetApiName());
            if (!api) {
                TFLEventLog::Error("Api not configured")("api", Config.GetApiName());
                return false;
            }
            const auto r = SendEncodeRequest(api, plain);
            if (!r.IsSuccess()) {
                TFLEventLog::Error("cannot encrypt");
                return false;
            }
            encoded = r.GetEncoded();
            return true;
        }

        virtual bool Decrypt(const TString& encoded, TString& plain) const final {
            const auto api = Server.GetSenderPtr(Config.GetApiName());
            if (!api) {
                TFLEventLog::Error("Api not configured")("api", Config.GetApiName());
                return false;
            }
            const auto r = SendDecodeRequest(api, encoded);
            if (!r.IsSuccess()) {
                TFLEventLog::Error("cannot decrypt");
                return false;
            }
            plain = r.GetPlain();
            return true;
        }

        IExternalCipher(const IBaseServer& server, const TConfig& config)
            : Server(server)
            , Config(config)
        {
        }
    };

    class TExternalCipher: public IExternalCipher<TEncodeRequest, TDecodeRequest, TExternalCipherConfig> {
    private:
        using TBase = IExternalCipher<TEncodeRequest, TDecodeRequest, TExternalCipherConfig>;

    protected:
        virtual IAbstractCipher::TPtr DoCreateNewVersion() const override {
            return MakeAtomicShared<TExternalCipher>(Server, Config);
        }
        virtual TEncodeRequest::TResponse SendEncodeRequest(const NExternalAPI::TSender::TPtr api, const TString& plain) const override {
            return api->SendRequest<TEncodeRequest>(plain, Config);
        }

        virtual TDecodeRequest::TResponse SendDecodeRequest(const NExternalAPI::TSender::TPtr api, const TString& plain) const override {
            return api->SendRequest<TDecodeRequest>(plain, Config);
        }

    public:
        virtual bool NeedRotate() const override {
            return false;
        }

        using TBase::TBase;
    };
}
