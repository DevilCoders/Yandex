#pragma once
#include "abstract.h"

namespace NCS {

    namespace NObfuscator {

        class TFakeObfuscator: public IObfuscatorWithRules {
        private:
            using TBase = IObfuscatorWithRules;

            static TFactory::TRegistrator<TFakeObfuscator> Registrator;
            virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const override {
                NFrontend::TScheme result = TBase::DoGetScheme(server);
                return result;
            }

            virtual TString DoObfuscate(const TStringBuf str) const override {
                return TString(str);
            }

            virtual bool DoObfuscateInplace(NJson::TJsonValue& /*json*/) const override {
                return true;
            }

        public:
            static TString GetTypeName() {
                return "fake_obfuscator";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }
            virtual NJson::TJsonValue DoSerializeToJson() const override {
                NJson::TJsonValue result = TBase::DoSerializeToJson();
                return result;
            }
            virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override {
                if (!TBase::DoDeserializeFromJson(jsonInfo)) {
                    return false;
                }
                return true;
            }
        };

    }
}
