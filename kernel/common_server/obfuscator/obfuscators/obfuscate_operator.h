#pragma once
#include <kernel/common_server/api/history/common.h>
#include <kernel/common_server/ciphers/abstract.h>
#include <kernel/common_server/abstract/frontend.h>

namespace NCS {
    namespace NObfuscator {

        class IObfuscateOperator {
        public:
            enum class EPolicy {
                ObfuscateWithText,
                ObfuscateWithFnvHash,
                Crypto,
                Tokenize,
                NoObfuscation,
            };

            using TFactory = NObjectFactory::TObjectFactory<IObfuscateOperator, TString>;
            using TPtr = TAtomicSharedPtr<IObfuscateOperator>;

            virtual NJson::TJsonValue SerializeToJson() const = 0;
            virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) = 0;
            virtual NFrontend::TScheme GetScheme(const IBaseServer& server) const = 0;
            virtual TString GetClassName() const = 0;

            virtual TString Obfuscate(const TStringBuf str) const = 0;
            virtual ~IObfuscateOperator() = default;
        };

        class TObfuscateWithText: public IObfuscateOperator {
        private:
            CS_ACCESS(TObfuscateWithText, TString, ObfuscatedText, "Hidden data");

            static TFactory::TRegistrator<TObfuscateWithText> Registrator;

        public:
            virtual NJson::TJsonValue SerializeToJson() const override;
            virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;
            virtual NFrontend::TScheme GetScheme(const IBaseServer& /*server*/) const override;
            virtual TString Obfuscate(const TStringBuf /*str*/) const override;
            static TString GetTypeName() {
                return ToString(EPolicy::ObfuscateWithText);
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }
        };

        class TObfuscateWithHash: public IObfuscateOperator {
        private:
            static TFactory::TRegistrator<TObfuscateWithHash> Registrator;

        public:
            virtual NJson::TJsonValue SerializeToJson() const override;
            virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;
            virtual NFrontend::TScheme GetScheme(const IBaseServer& server) const override;
            virtual TString Obfuscate(const TStringBuf str) const override;

            static TString GetTypeName() {
                return ToString(EPolicy::ObfuscateWithFnvHash);
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }
        };

        class TNoObfuscate: public IObfuscateOperator {
        private:
            static TFactory::TRegistrator<TNoObfuscate> Registrator;

        public:
            virtual NJson::TJsonValue SerializeToJson() const override;
            virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;
            virtual NFrontend::TScheme GetScheme(const IBaseServer& server) const override;
            virtual TString Obfuscate(const TStringBuf str) const override;

            static TString GetTypeName() {
                return ToString(EPolicy::NoObfuscation);
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }
        };

        class TObfuscateWithCipher: public IObfuscateOperator {
        private:
            CSA_READONLY_DEF(TString, CipherName);
            static TFactory::TRegistrator<TObfuscateWithCipher> Registrator;

        public:
            virtual NJson::TJsonValue SerializeToJson() const override;
            virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;
            virtual NFrontend::TScheme GetScheme(const IBaseServer& server) const override;
            virtual TString Obfuscate(const TStringBuf str) const override;

            static TString GetTypeName() {
                return ToString(EPolicy::Crypto);
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }
        };

        class TObfuscateWithTokenizator: public IObfuscateOperator {
        private:
            CSA_READONLY_DEF(TString, TokenizitorName);
            static TFactory::TRegistrator<TObfuscateWithTokenizator> Registrator;
        public:
            virtual NJson::TJsonValue SerializeToJson() const override;
            virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;
            virtual NFrontend::TScheme GetScheme(const IBaseServer& server) const override;
            virtual TString Obfuscate(const TStringBuf str) const override;

            static TString GetTypeName() {
                return ToString(EPolicy::Tokenize);
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }
        };
    }
}
