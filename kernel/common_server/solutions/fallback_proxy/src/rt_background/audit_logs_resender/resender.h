#pragma once

#include <fintech/backend-kotlin/services/audit/api/message.pb.h>
#include <kernel/common_server/solutions/fallback_proxy/src/rt_background/queue_resender/process.h>

namespace NCS {
    namespace NFallbackProxy {

        class TAuditKafkaReaderProcess: public IQueueResenderProcess<NExternalAPI::TServiceApiHttpDirectRequest> {
        private:
            using TMessage = NExternalAPI::TServiceApiHttpDirectRequest;
            using TBase = IQueueResenderProcess<TMessage>;
            using TRequestsMap = TBase::TRequestsMap;
            using TAuditProtoMessage = ru::yandex::fintech::audit::api::Message;
            static TFactory::TRegistrator<TAuditKafkaReaderProcess> Registrator;
            CSA_DEFAULT(TAuditKafkaReaderProcess, TString, TargetPath);

        private:
            virtual TMaybe<TMessage> ConvertMessage(NCS::IPQMessage::TPtr message) const override;

        protected:
            virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const override;
            virtual NJson::TJsonValue DoSerializeToJson() const override;
            virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;

        public:
            static TString GetTypeName() {
                return "audit_kafka_reader";
            }

            virtual TString GetType() const override {
                return GetTypeName();
            }
        };

    }
}
