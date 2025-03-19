#pragma once
#include "abstract.h"

#include <kernel/common_server/obfuscator/obfuscators/obfuscate_operator.h>

namespace NCS {

    namespace NObfuscator {

        class TObfuscatorWithPolicy: public IObfuscatorWithRules {
        public:
            static TString GetTypeName() {
                return "with_policy_obfuscator";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }

            virtual NJson::TJsonValue DoSerializeToJson() const override;

            virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;

        private:
            using TBase = IObfuscatorWithRules;

            virtual TString DoObfuscate(const TStringBuf str) const override;

            static TFactory::TRegistrator<TObfuscatorWithPolicy> Registrator;

            virtual NFrontend::TScheme DoGetScheme(const IBaseServer& /*server*/) const override;

        private:
            CSA_DEFAULT(TObfuscatorWithPolicy, TInterfaceContainer<IObfuscateOperator>, ObfuscateOperator);
        };

    }

}
