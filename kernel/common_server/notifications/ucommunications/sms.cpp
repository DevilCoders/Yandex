#include "sms.h"
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/library/logging/events.h>

#include "request.h"

TUCommNotifierConfig::TFactory::TRegistrator<TUCommNotifierConfig> TUCommNotifierConfig::Registrator("ucommunications");

TUCommNotifier::TUCommNotifier(const TUCommNotifierConfig& config)
    : IFrontendNotifier(config)
    , Config(config)
{
}

TUCommNotifier::~TUCommNotifier() {
}

bool TUCommNotifier::DoStart(const NCS::IExternalServicesOperator& context) {
    CHECK_WITH_LOG(!Sender);
    Sender = context.GetSenderPtr(Config.GetApiName());
    return true;
}

bool TUCommNotifier::DoStop() {
    Sender = nullptr;
    return true;
}

IFrontendNotifier::TResult::TPtr TUCommNotifier::DoNotify(const TMessage& message, const TContext& context) const {
    CHECK_WITH_LOG(Sender);

    if (context.GetRecipients().size() != 1) {
        return MakeAtomicShared<TResult>("Works with only one contact");
    }
    TSendSmsRequest request;
    request.SetIntent(Config.GetIntent()).SetSender(Config.GetSender());
    request.SetMessage(message.GetBody()).SetPhoneId(context.GetRecipients().front().GetPhone());
    auto result = Sender->SendRequest(request);
    if (result.IsSuccess()) {
        return MakeAtomicShared<TResult>("");
    } else {
        return MakeAtomicShared<TResult>(result.GetResponseLog().SerializeToString());
    }
}


IFrontendNotifier::TPtr TUCommNotifierConfig::Construct() const {
    return MakeAtomicShared<TUCommNotifier>(*this);
}
