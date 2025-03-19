#include "internal.h"
#include <library/cpp/logger/global/global.h>
#include <util/string/join.h>

bool TInternalNotifier::DoStart(const NCS::IExternalServicesOperator& /*context*/) {
    return true;
}

bool TInternalNotifier::DoStop() {
    return true;
}

IFrontendNotifier::TResult::TPtr TInternalNotifier::DoNotify(const TMessage& message, const TContext& context) const {
    const TRecipients& recipients = context.GetRecipients();
    for (size_t i = 0; i < recipients.size(); ++i) {
        SendGlobalMessage<TInternalNotificationMessage>(recipients[i].GetPassportUid(), message.GetBody());
    }
    return nullptr;
}

TInternalNotifierConfig::TFactory::TRegistrator<TInternalNotifierConfig> TInternalNotifierConfig::Registrator("internal");


IFrontendNotifier::TPtr TInternalNotifierConfig::Construct() const {
    return MakeAtomicShared<TInternalNotifier>(*this);
}
