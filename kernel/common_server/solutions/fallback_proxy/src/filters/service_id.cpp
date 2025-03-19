#include "service_id.h"

#include <kernel/common_server/solutions/fallback_proxy/src/pq_message/message.h>
#include <fintech/backend-kotlin/services/audit/api/message.pb.h>

namespace NCS {
    namespace NFallbackProxy {
        TAuditServiceIdFilter::TFactory::TRegistrator<TAuditServiceIdFilter> TAuditServiceIdFilter::Registrator(TAuditServiceIdFilter::GetTypeName());

        bool TAuditServiceIdFilter::DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            if (!TJsonProcessor::Read(jsonInfo, "service_id", ServiceId, /* mustBe = */ true, /* mayBeEmpty = */ false)) {
                return false;
            }
            return true;
        }

        NJson::TJsonValue TAuditServiceIdFilter::DoSerializeToJson() const {
            NJson::TJsonValue result = NJson::JSON_MAP;
            TJsonProcessor::Write(result, "service_id", ServiceId);
            return result;
        }

        bool TAuditServiceIdFilter::DoAccept(const IPQMessage::TPtr message) const {
            ru::yandex::fintech::audit::api::Message msg;
            if (!msg.ParseFromArray(message->GetContent().AsCharPtr(), message->GetContent().Length())) {
                TFLEventLog::Error("Could not parse proto message");
                return false;
            }

            return msg.Getservice() == ServiceId;
        }

        NFrontend::TScheme TAuditServiceIdFilter::DoGetScheme(const IBaseServer& /* server */) const {
            NFrontend::TScheme result;
            result.Add<TFSString>("service_id").SetRequired(true);
            return result;
        }
    }
}
