#pragma once

#include <kernel/common_server/notifications/abstract/abstract.h>
#include <kernel/common_server/notifications/mail/mail.h>
#include <kernel/common_server/library/tvm_services/abstract/abstract.h>
#include <kernel/common_server/abstract/frontend.h>

namespace NCS {
    class IExternalServicesOperator;
};

template <class TNotifier, class TNotifierConfig>
class TNotifierHolder {
public:
    TNotifierHolder(const TNotifierConfig& config, const NCS::IExternalServicesOperator& context = NCS::TExternalServicesOperator())
        : Notifier(config)
    {
        Notifier.Start(context);
    }
    ~TNotifierHolder() {
        Notifier.Stop();
    }
    IFrontendNotifier::TResult::TPtr SendTestMessage(
            const IFrontendNotifier::TMessage& message,
            const IFrontendNotifier::TRecipients& recipients) {
        return SendTestMessage(message, IFrontendNotifier::TContext().SetRecipients(recipients));
    }
    IFrontendNotifier::TResult::TPtr SendTestMessage(
            const IFrontendNotifier::TMessage& message,
            const IFrontendNotifier::TContext& context) {
        return Notifier.Notify(message, "", context);
    }
private:
    TNotifier Notifier;
};
