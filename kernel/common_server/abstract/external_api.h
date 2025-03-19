#pragma once
#include <util/generic/ptr.h>
#include <kernel/common_server/library/tvm_services/abstract/abstract.h>
#include <kernel/common_server/library/persistent_queue/abstract/pq.h>
#include <kernel/common_server/library/persistent_queue/abstract/config.h>

namespace NCS {

    class IExternalServicesOperator {
    public:
        virtual ~IExternalServicesOperator() = default;
        virtual TSet<TString> GetQueueNames() const = 0;
        virtual IPQClient::TPtr GetQueuePtr(const TString& serviceId) const = 0;
        IPQClient::TPtr GetQueuePtrUnsafe(const TString& serviceId) const;

        virtual TSet<TString> GetAbstractExternalAPINames() const = 0;
        virtual NExternalAPI::TSender::TPtr GetSenderPtr(const TString& serviceName) const = 0;
        NExternalAPI::TSender::TPtr GetSenderPtrUnsafe(const TString& serviceName) const;

        virtual void StartExternalServiceClients() = 0;
        virtual void StopExternalServiceClients() = 0;
    };

    class TExternalServicesOperator: public virtual IExternalServicesOperator {
    private:
        TMap<TString, NExternalAPI::TSender::TPtr> AbstractExternalAPI;
        TMap<TString, IPQClient::TPtr> ExternalQueue;
    public:
        TExternalServicesOperator() = default;
        void AddService(const TString& serviceName, const NExternalAPI::TSenderConfig& config,
            const NExternalAPI::IRequestCustomizationContext* cContext = nullptr);
        void AddQueue(const TPQClientConfigContainer& config, const IPQConstructionContext& cContext);

        virtual TSet<TString> GetAbstractExternalAPINames() const override;
        virtual NExternalAPI::TSender::TPtr GetSenderPtr(const TString& serviceName) const override;

        virtual TSet<TString> GetQueueNames() const override;
        virtual IPQClient::TPtr GetQueuePtr(const TString& serviceId) const override;

        virtual void StartExternalServiceClients() override;

        virtual void StopExternalServiceClients() override;
    };

} // namespace
