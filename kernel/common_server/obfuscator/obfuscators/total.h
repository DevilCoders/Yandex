#pragma once
#include "abstract.h"

namespace NCS {

    namespace NObfuscator {

        class TTotalObfuscator: public IObfuscator {
        private:
            static TFactory::TRegistrator<TTotalObfuscator> Registrator;
            virtual NFrontend::TScheme DoGetScheme(const IBaseServer& /*server*/) const override {
                NFrontend::TScheme result;
                return result;
            }

            virtual TString DoObfuscate(const TStringBuf str) const override;

        public:
            static TString GetTypeName() {
                return "total_obfuscator";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }
            virtual NJson::TJsonValue DoSerializeToJson() const override {
                NJson::TJsonValue result = NJson::JSON_MAP;
                return result;
            }
            virtual bool DoDeserializeFromJson(const NJson::TJsonValue& /*jsonInfo*/) override {
                return true;
            }

            virtual bool IsMatch(const TObfuscatorKey& /*key*/) const override {
                return true;
            }
            virtual size_t GetRulesCount() const override {
                return 0;
            }
        };

    }
}
