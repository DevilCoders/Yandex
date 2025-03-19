#include "object.h"
#include "policies/simple.h"

namespace NCS {

    namespace NPropositions {

        NCS::NPropositions::TDBProposition& TDBProposition::SetProposedObject(const TProposedActionContainer& container) {
            ProposedObject = container;
            PropositionCategoryId = container.GetCategoryId();
            PropositionObjectId = container.GetObjectId();
            return *this;
        }

        TString TDBProposition::GetTitle() const {
            return ProposedObject.GetClassName() + ": " + PropositionCategoryId + ": " + PropositionObjectId + ": " + ::ToString(PropositionId);
        }

        NJson::TJsonValue TDBProposition::SerializeToJson() const {
            NJson::TJsonValue result = ProposedObject.SerializeToJson();
            TJsonProcessor::Write(result, "_title", GetTitle());
            TJsonProcessor::Write(result, "proposition_id", PropositionId);
            TJsonProcessor::Write(result, "proposition_object_id", PropositionObjectId);
            TJsonProcessor::Write(result, "proposition_category_id", PropositionCategoryId);
            TJsonProcessor::Write(result, "revision", Revision);
            TJsonProcessor::WriteObject(result, "proposition_policy", PropositionPolicy);
            return result;
        }

        bool TDBProposition::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            if (!TJsonProcessor::Read(jsonInfo, "proposition_id", PropositionId)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "revision", Revision)) {
                return false;
            }

            TProposedActionContainer c;
            if (!c.DeserializeFromJson(jsonInfo)) {
                return false;
            }
            SetProposedObject(c);

            if (!TJsonProcessor::ReadObject(jsonInfo, "proposition_policy", PropositionPolicy)) {
                return false;
            }
            return true;
        }

        NStorage::TTableRecord TDBProposition::SerializeToTableRecord() const {
            NStorage::TTableRecord result;
            result.SetNotEmpty("proposition_id", PropositionId);
            result.SetNotEmpty("proposition_object_id", PropositionObjectId);
            result.SetNotEmpty("proposition_category_id", PropositionCategoryId);
            result.SetNotEmpty("revision", Revision);
            result.SetNotEmpty("class_name", ProposedObject.GetClassName());
            result.SetBytes("data", ProposedObject.SerializeToString());
            result.SetNotEmpty("proposition_policy", PropositionPolicy.SerializeToJson().GetStringRobust());
            return result;
        }

        bool TDBProposition::DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
            READ_DECODER_VALUE(decoder, values, PropositionId);
            READ_DECODER_VALUE(decoder, values, PropositionObjectId);
            READ_DECODER_VALUE(decoder, values, PropositionCategoryId);
            READ_DECODER_VALUE(decoder, values, Revision);
            NJson::TJsonValue policyJson;
            READ_DECODER_VALUE_JSON(decoder, values, policyJson, PropositionPolicy);
            if (!PropositionPolicy.DeserializeFromJson(policyJson)) {
                return false;
            }
            {
                TString className;
                READ_DECODER_VALUE_TEMP(decoder, values, className, ClassName);
                TString data;
                if (!decoder.GetValueBytes(decoder.GetData(), values, data)) {
                    return false;
                }
                return ProposedObject.DeserializeFromString(className, data);
            }
        }

        NFrontend::TScheme TDBProposition::GetScheme(const IBaseServer& server) {
            NFrontend::TScheme result = TProposedActionContainer::GetScheme(server);
            result.Add<TFSNumeric>("proposition_id").SetReadOnly(true);
            result.Add<TFSNumeric>("revision").SetReadOnly(true);
            result.Add<TFSString>("proposition_object_id").SetReadOnly(true);
            result.Add<TFSString>("proposition_category_id").SetReadOnly(true);
            result.Add<TFSStructure>("proposition_policy").SetStructure(TPropositionPolicyContainer::GetScheme(server)).SetRequired(false);
            result.Add<TFSString>("_title").SetReadOnly(true);
            return result;
        }

        bool TDBProposition::CheckPolicy(const IBaseServer& server) const {
            if (!PropositionPolicy.IsValid(server)) {
                return false;
            }
            return true;
        }
    }
}
