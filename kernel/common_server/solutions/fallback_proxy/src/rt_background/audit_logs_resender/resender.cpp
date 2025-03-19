#include "resender.h"

namespace NCS {
    namespace NFallbackProxy {
        TAuditKafkaReaderProcess::TFactory::TRegistrator<TAuditKafkaReaderProcess> TAuditKafkaReaderProcess::Registrator(TAuditKafkaReaderProcess::GetTypeName());

        TMaybe<TAuditKafkaReaderProcess::TMessage> TAuditKafkaReaderProcess::ConvertMessage(NCS::IPQMessage::TPtr message) const {
            TAuditProtoMessage protoMessage;
            if (!protoMessage.ParseFromArray(message->GetContent().AsCharPtr(), message->GetContent().Length())) {
                TFLEventLog::Signal("Could not parse message content into protobuf");
                return Nothing();
            }

            NNeh::THttpRequest request;
            request.SetUri(TargetPath);

            if (GetCgiMessageId()) {
                request.AddCgiData(GetCgiMessageId(), message->GetMessageId());
            }

            NJson::TJsonValue jsonValue;
            NProtobufJson::Proto2Json(protoMessage, jsonValue["json_request_info"]);
            request.SetPostData(jsonValue);

            return request;
        }

        NFrontend::TScheme TAuditKafkaReaderProcess::DoGetScheme(const IBaseServer& server) const {
            NFrontend::TScheme result = TBase::DoGetScheme(server);
            result.Add<TFSString>("target_path");
            return result;
        }

        NJson::TJsonValue TAuditKafkaReaderProcess::DoSerializeToJson() const {
            auto result = TBase::DoSerializeToJson();
            TJsonProcessor::Write(result, "target_path", TargetPath);
            return result;
        }

        bool TAuditKafkaReaderProcess::DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            if (!TJsonProcessor::Read(jsonInfo, "target_path", TargetPath, /* mustBe = */ true, /* mayBeEmpty = */ false)) {
                return false;
            }

            return TBase::DoDeserializeFromJson(jsonInfo);
        }

    }
}
