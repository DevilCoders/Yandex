#include "external_api.h"
#include <kernel/common_server/util/algorithm/container.h>

namespace NCS {

TSet<TString> TExternalServicesOperator::GetAbstractExternalAPINames() const {
    return MakeSet(NContainer::Keys(AbstractExternalAPI));
}

IPQClient::TPtr IExternalServicesOperator::GetQueuePtrUnsafe(const TString& serviceId) const {
    auto result = GetQueuePtr(serviceId);
    CHECK_WITH_LOG(!!result);
    return result;
}

NExternalAPI::TSender::TPtr IExternalServicesOperator::GetSenderPtrUnsafe(const TString& serviceName) const {
    auto ptr = GetSenderPtr(serviceName);
    CHECK_WITH_LOG(ptr);
    return ptr;
}

NExternalAPI::TSender::TPtr TExternalServicesOperator::GetSenderPtr(const TString& serviceName) const {
    auto it = AbstractExternalAPI.find(serviceName);
    if(it == AbstractExternalAPI.end()) {
        return nullptr;
    }
    return it->second;
}

TSet<TString> TExternalServicesOperator::GetQueueNames() const {
    return MakeSet(NContainer::Keys(ExternalQueue));
}

IPQClient::TPtr TExternalServicesOperator::GetQueuePtr(const TString& serviceId) const {
    auto it = ExternalQueue.find(serviceId);
    if (it == ExternalQueue.end()) {
        return nullptr;
    }
    return it->second;
}

void TExternalServicesOperator::StartExternalServiceClients() {
    for (auto&& [_, i] : ExternalQueue) {
        CHECK_WITH_LOG(i->Start());
    }
}

void TExternalServicesOperator::StopExternalServiceClients() {
    for (auto&& [_, i] : ExternalQueue) {
        CHECK_WITH_LOG(i->Stop());
    }
}

void TExternalServicesOperator::AddService(const TString& serviceName, const NExternalAPI::TSenderConfig& config, const NExternalAPI::IRequestCustomizationContext* cContext) {
    auto ptr = MakeAtomicShared<NExternalAPI::TSender>(config, serviceName, cContext);
    AbstractExternalAPI.emplace(serviceName, std::move(ptr));
}

void TExternalServicesOperator::AddQueue(const TPQClientConfigContainer& config, const IPQConstructionContext& cContext) {
    CHECK_WITH_LOG(!!config);
    auto pqClient = config->Construct(cContext);
    CHECK_WITH_LOG(!!pqClient);
    CHECK_WITH_LOG(ExternalQueue.emplace(pqClient->GetClientId(), pqClient).second);
}

}
