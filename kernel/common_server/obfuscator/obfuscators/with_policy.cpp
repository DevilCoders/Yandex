#include "with_policy.h"

#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/ciphers/abstract.h>

namespace NCS {
    namespace NObfuscator {
        TObfuscatorWithPolicy::TFactory::TRegistrator<TObfuscatorWithPolicy> TObfuscatorWithPolicy::Registrator(TObfuscatorWithPolicy::GetTypeName());

        TString TObfuscatorWithPolicy::DoObfuscate(const TStringBuf str) const {
            CHECK_WITH_LOG(!!ObfuscateOperator);
            return ObfuscateOperator->Obfuscate(str);
        }

        NFrontend::TScheme TObfuscatorWithPolicy::DoGetScheme(const IBaseServer& server) const {
            NFrontend::TScheme result = TBase::DoGetScheme(server);
            result.Add<TFSStructure>("obfuscate_operator").SetStructure(ObfuscateOperator.GetScheme(server));
            return result;
        }

        NJson::TJsonValue TObfuscatorWithPolicy::DoSerializeToJson() const {
            NJson::TJsonValue result = TBase::DoSerializeToJson();
            TJsonProcessor::WriteObject(result, "obfuscate_operator", ObfuscateOperator);
            return result;
        }

        bool TObfuscatorWithPolicy::DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            if (!TBase::DoDeserializeFromJson(jsonInfo)) {
                return false;
            }

            if (!TJsonProcessor::ReadObject(jsonInfo, "obfuscate_operator", ObfuscateOperator)) {
                return false;
            }
            return true;
        }

    }
}
