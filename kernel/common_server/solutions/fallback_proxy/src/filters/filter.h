#pragma once

#include <kernel/common_server/library/persistent_queue/abstract/pq.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NCS {
    namespace NFallbackProxy {
        class IPQMessagesFilter {
        private:
            virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) = 0;
            virtual NJson::TJsonValue DoSerializeToJson() const = 0;
            virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const = 0;

            virtual bool DoAccept(const IPQMessage::TPtr message) const = 0;

        public:
            using TFactory = NObjectFactory::TObjectFactory<IPQMessagesFilter, TString>;
            using TPtr = TAtomicSharedPtr<IPQMessagesFilter>;
            virtual ~IPQMessagesFilter() = default;

            bool Accept(const IPQMessage::TPtr message) const;

            bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
            NJson::TJsonValue SerializeToJson() const;
            NFrontend::TScheme GetScheme(const IBaseServer& server) const;

            virtual TString GetClassName() const = 0;

        };

        class TPQMessagesFakeFilter: public IPQMessagesFilter {
        private:
            using TBase = IPQMessagesFilter;
            static TFactory::TRegistrator<TPQMessagesFakeFilter> Registrator;

            virtual bool DoAccept(const IPQMessage::TPtr message) const override;

        protected:
            virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;
            virtual NJson::TJsonValue DoSerializeToJson() const override;
            virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const override;

        public:
            static TString GetTypeName() {
                return "fake_messages_filter";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }
        };

        class TPQMessagesFilterContainer: public TBaseInterfaceContainer<IPQMessagesFilter> {
        private:
            using TBase = TBaseInterfaceContainer<IPQMessagesFilter>;
        };
    }
}
