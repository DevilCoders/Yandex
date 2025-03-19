#include "verdict.h"
#include <kernel/common_server/util/instant_model.h>

namespace NCS {

    namespace NPropositions {

        NJson::TJsonValue TDBVerdict::SerializeToJson() const {
            NJson::TJsonValue result = NJson::JSON_MAP;
            result.InsertValue("_title", ::ToString(PropositionId) + ": " + ::ToString(PropositionRevision) + ": " + ::ToString(VerdictId));
            TJsonProcessor::Write(result, "verdict_id", VerdictId);
            TJsonProcessor::Write(result, "proposition_id", PropositionId);
            TJsonProcessor::Write(result, "proposition_revision", PropositionRevision);
            TJsonProcessor::Write(result, "comment", Comment);
            TJsonProcessor::Write(result, "system_user_id", SystemUserId);
            TJsonProcessor::WriteAsString(result, "verdict", Verdict);
            TJsonProcessor::Write(result, "verdict_instant", VerdictInstant);
            return result;
        }

        bool TDBVerdict::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            if (!TJsonProcessor::Read(jsonInfo, "proposition_id", PropositionId)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "proposition_revision", PropositionRevision)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "comment", Comment)) {
                return false;
            }
            if (!TJsonProcessor::ReadFromString(jsonInfo, "verdict", Verdict)) {
                return false;
            }
            VerdictInstant = ModelingNow();
            return true;
        }

        NStorage::TTableRecord TDBVerdict::SerializeToTableRecord() const {
            NStorage::TTableRecord result;
            result.SetNotEmpty("verdict_id", VerdictId);
            result.SetNotEmpty("proposition_id", PropositionId);
            result.SetNotEmpty("proposition_revision", PropositionRevision);
            result.SetNotEmpty("comment", Comment);
            result.SetNotEmpty("system_user_id", SystemUserId);
            result.SetNotEmpty("verdict", ::ToString(Verdict));
            result.SetNotEmpty("verdict_instant", VerdictInstant.Seconds());
            return result;
        }

        bool TDBVerdict::DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
            READ_DECODER_VALUE(decoder, values, VerdictId);
            READ_DECODER_VALUE(decoder, values, PropositionId);
            READ_DECODER_VALUE(decoder, values, PropositionRevision);
            READ_DECODER_VALUE(decoder, values, Comment);
            READ_DECODER_VALUE(decoder, values, SystemUserId);
            READ_DECODER_VALUE(decoder, values, Verdict);
            READ_DECODER_VALUE_INSTANT(decoder, values, VerdictInstant);
            return true;
        }

        NFrontend::TScheme TDBVerdict::GetScheme(const IBaseServer& /*server*/) {
            NFrontend::TScheme result;
            result.Add<TFSNumeric>("verdict_id").SetReadOnly(true);
            result.Add<TFSNumeric>("proposition_id").SetReadOnly(true);
            result.Add<TFSNumeric>("proposition_revision").SetReadOnly(true);
            result.Add<TFSString>("comment");
            result.Add<TFSString>("system_user_id").SetReadOnly(true);
            result.Add<TFSVariants>("verdict").InitVariants<EVerdict>();
            result.Add<TFSNumeric>("verdict_instant").SetVisual(TFSNumeric::DateTime).SetReadOnly(true);
            result.Add<TFSString>("_title").SetReadOnly(true);
            return result;
        }
    }
}
