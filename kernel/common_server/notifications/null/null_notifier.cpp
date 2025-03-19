#include "null_notifier.h"

bool TNullNotifier::DoStart(const NCS::IExternalServicesOperator& /*context*/) {
    return true;
}

bool TNullNotifier::DoStop() {
    return true;
}

IFrontendNotifier::TResult::TPtr TNullNotifier::DoNotify(const TMessage& /*message*/, const TContext& /*context*/) const {
    return nullptr;
}

TNullNotifierConfig::TFactory::TRegistrator<TNullNotifierConfig> TNullNotifierConfig::Registrator("null");

IFrontendNotifier::TPtr TNullNotifierConfig::Construct() const {
    return MakeAtomicShared<TNullNotifier>(*this);
}
