#pragma once
#include "abstract.h"

namespace NCS {

    namespace NObfuscator {
        class TWhiteListObfuscator: public IObfuscatorWithRules {
        private:
            using TBase = IObfuscatorWithRules;
            CSA_DEFAULT(TWhiteListObfuscator, TSet<TString>, Keys);
            CS_ACCESS(TWhiteListObfuscator, i32, MaxDeep, 32);
            CS_ACCESS(TWhiteListObfuscator, TString, ObfuscatedText, "Hidden data");

            static TFactory::TRegistrator<TWhiteListObfuscator> Registrator;

            TString ObfuscateXml(const TStringBuf strBuf) const;
            TString ObfuscateJson(const TStringBuf str) const;
            bool ObfuscateJson(NJson::TJsonValue& json, i32 curDeep) const;
            bool ObfuscateXml(NXml::TNode& node, i32 curDeep) const;

            virtual TString DoObfuscate(const TStringBuf str) const override {
                if (Keys.contains("*")) {
                    return TString(str);
                }
                switch (GetContentType()) {
                    case EContentType::Json:
                        return ObfuscateJson(str);
                    case EContentType::Xml:
                        return ObfuscateXml(str);
                    case EContentType::Text:
                        return ObfuscatedText;
                    default:
                        return ObfuscatedText;
                }
            }

        protected:
            virtual NFrontend::TScheme DoGetScheme(const IBaseServer& /*server*/) const override;

        public:
            static TString GetTypeName() {
                return "white_list_obfuscator";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }

            virtual NJson::TJsonValue DoSerializeToJson() const override;
            virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;
        };

    }
}
