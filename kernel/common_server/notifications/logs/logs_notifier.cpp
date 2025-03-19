#include "logs_notifier.h"
#include <library/cpp/logger/global/global.h>
#include <util/string/join.h>

bool TLogsNotifier::DoStart(const NCS::IExternalServicesOperator& /*context*/) {
    return true;
}

bool TLogsNotifier::DoStop() {
    return true;
}

IFrontendNotifier::TResult::TPtr TLogsNotifier::DoNotify(const TMessage& message, const TContext& context) const {
    TString recipientsStr;

    const TRecipients& recipients = context.GetRecipients();

    for (size_t i = 0; i < recipients.size(); ++i) {
        const TUserContacts& contact = recipients[i];
        recipientsStr += contact.GetEmail() + ":" + contact.GetPassportUid();
        if (i != recipients.size() - 1) {
            recipientsStr += ", ";
        }
    }

    TFLEventLog::Log(message.GetBody(), Config.GetLogPriority())
        ("title", message.GetTitle())("header", message.GetHeader())("recipients", recipientsStr);
    return nullptr;
}

TLogsNotifierConfig::TFactory::TRegistrator<TLogsNotifierConfig> TLogsNotifierConfig::Registrator("logs");


IFrontendNotifier::TPtr TLogsNotifierConfig::Construct() const {
    return MakeAtomicShared<TLogsNotifier>(*this);
}
