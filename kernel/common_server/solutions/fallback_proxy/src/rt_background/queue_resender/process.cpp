#include "process.h"
#include <kernel/common_server/library/tvm_services/abstract/request/direct.h>
#include <kernel/common_server/solutions/fallback_proxy/src/pq_message/message.h>

namespace NCS {
    namespace NFallbackProxy {

        TResenderProcess::TFactory::TRegistrator<TResenderProcess> TResenderProcess::Registrator(TResenderProcess::GetTypeName());

        TMaybe<TResenderProcess::TMessage> TResenderProcess::ConvertMessage(NCS::IPQMessage::TPtr message) const {
            TMaybe<TRequestPQMessage> rpqMessage = TRequestPQMessage::BuildFromPQMessage(*message);
            if (rpqMessage) {
                auto request = rpqMessage->GetRequest();
                if (GetCgiMessageId()) {
                    request.AddCgiData(GetCgiMessageId(), message->GetMessageId());
                }
                return TResenderProcess::TMessage(request);
            }
            TFLEventLog::Signal("could not parse into rpq message");
            return Nothing();
        }

        NFrontend::TScheme TResenderProcess::DoGetScheme(const IBaseServer& server) const {
            return TBase::DoGetScheme(server);
        }

        NJson::TJsonValue TResenderProcess::DoSerializeToJson() const {
            return TBase::DoSerializeToJson();
        }

        bool TResenderProcess::DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            return TBase::DoDeserializeFromJson(jsonInfo);
        }
    }
}
