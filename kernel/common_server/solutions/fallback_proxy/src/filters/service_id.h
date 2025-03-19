#pragma once

#include "filter.h"

namespace NCS {
    namespace NFallbackProxy {
        class TAuditServiceIdFilter: public IPQMessagesFilter {
        private:
            CSA_READONLY_DEF(TString, ServiceId);
            using TBase = IPQMessagesFilter;
            static TFactory::TRegistrator<TAuditServiceIdFilter> Registrator;

            virtual bool DoAccept(const IPQMessage::TPtr message) const override;

        protected:
            virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;
            virtual NJson::TJsonValue DoSerializeToJson() const override;
            virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const override;

        public:
            static TString GetTypeName() {
                return "audit_service_id_filter";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }
        };
    }
}
