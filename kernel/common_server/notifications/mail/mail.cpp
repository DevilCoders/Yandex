#include "mail.h"
#include <library/cpp/string_utils/base64/base64.h>
#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/library/async_proxy/async_delivery.h>
#include <kernel/common_server/library/json/builder.h>
#include <kernel/common_server/library/unistat/cache.h>

#include <util/system/env.h>
#include <util/stream/file.h>
#include <util/string/join.h>

#include <kernel/common_server/abstract/frontend.h>

#include "request.h"

TMailNotificationsConfig::TFactory::TRegistrator<TMailNotificationsConfig> TMailNotificationsConfig::Registrator("mail");

void TMailNotificationsConfig::DoInit(const TYandexConfig::Section* section) {
    AssertCorrectConfig(section->GetDirectives().GetValue("Account", Account), "Incorrect parameter 'Account' for mail notifications");
    OverridenRecipient = section->GetDirectives().Value<TString>("OverridenRecipient");
    section->GetDirectives().TryFillArray("AdditionalBccRecipients", AdditionalBccRecipients);
    SenderApiName = section->GetDirectives().Value<TString>("SenderApiName", SenderApiName);
}

void TMailNotificationsConfig::DoToString(IOutputStream& os) const {
    os << "Account: " << Account << Endl;
    os << "OverridenRecipient: " << OverridenRecipient << Endl;
    os << "AdditionalBccRecipients: " << JoinSeq(", ", AdditionalBccRecipients) << Endl;
    os << "SenderApiName: " << SenderApiName << Endl;
}

IFrontendNotifier::TPtr TMailNotificationsConfig::Construct() const {
    return MakeAtomicShared<TMailNotifier>(*this);
}

bool TMailNotificationsConfig::DeserializeFromJson(const NJson::TJsonValue& info) {
    JREAD_STRING_OPT(info, "account", Account);
    JREAD_STRING_OPT(info, "overriden_recipient", OverridenRecipient);
    JREAD_CONTAINER(info, "additional_bcc_recipients", AdditionalBccRecipients);
    JREAD_STRING_OPT(info, "sender_api_name", SenderApiName);
    return TBase::DeserializeFromJson(info);
}

NJson::TJsonValue TMailNotificationsConfig::SerializeToJson() const {
    NJson::TJsonValue result = TBase::SerializeToJson();
    JWRITE(result, "account", Account);
    JWRITE(result, "overriden_recipient", OverridenRecipient);
    TJsonProcessor::WriteContainerArray(result, "additional_bcc_recipients", AdditionalBccRecipients, false);
    JWRITE(result, "sender_api_name", SenderApiName);
    return result;
}

NFrontend::TScheme TMailNotificationsConfig::GetScheme(const IBaseServer& server) const {
    NFrontend::TScheme result = TBase::GetScheme(server);
    result.Add<TFSString>("account").SetDefault(Account);
    result.Add<TFSString>("overriden_recipient").SetDefault(OverridenRecipient);
    result.Add<TFSArray>("additional_bcc_recipients").SetElement<TFSString>();
    result.Add<TFSString>("sender_api_name").SetDefault(SenderApiName);
    return result;
}

TMailNotifier::TMailNotifier(const TMailNotificationsConfig& config)
    : IFrontendNotifier(config)
    , Config(config)
{
}

TMailNotifier::~TMailNotifier() {
}

bool TMailNotifier::DoStart(const NCS::IExternalServicesOperator& context) {
    Sender = context.GetSenderPtr(Config.GetSenderApiName());
    if (!Sender) {
        TFLEventLog::Error("Can't start TMailNotifier, sender not found")("sender_name", Config.GetSenderApiName());
        return false;
    }
    return true;
}

bool TMailNotifier::DoStop() {
    Sender = nullptr;
    return true;
}

IFrontendNotifier::TResult::TPtr TMailNotifier::DoNotify(const TMessage& message, const TContext& context) const {
    auto templateId = message.GetHeader();
    if (!templateId) {
        return MakeAtomicShared<TResult>("Missing template id");
    }
    auto grLog = TFLRecords::StartContext()("template_id", templateId);

    TVector<TString> recipientsEmails;
    if (Config.GetOverridenRecipient()) {
        recipientsEmails.push_back(Config.GetOverridenRecipient());
    } else {
        const TRecipients& recipients = context.GetRecipients();
        for (auto&& userInfo : recipients) {
            recipientsEmails.push_back(userInfo.GetEmail());
        }
    }

    TMap<TString, TMailRequest> requests;
    for (auto&& email : recipientsEmails) {
        TMailRequest request(email, Config.GetAccount(), templateId);
        request.SetBccEmails(Config.GetAdditionalBccRecipients());
        request.SetArgs(message.GetBody());
        if (message.GetAdditionalInfo().IsMap() && message.GetAdditionalInfo().Has("attachments")) {
            if (!TJsonProcessor::ReadObjectsContainer(message.GetAdditionalInfo(), "attachments", request.MutableAttachments())) {
                TFLEventLog::Error("Can't read attachment in email");
            }
        }
        requests.emplace(email, std::move(request));
    }

    auto responses = Sender->SendPack(requests);
    for (const auto& [email, resp] : responses) {
        if (!resp.IsSuccess()) {
            return MakeAtomicShared<TResult>("Failed request");
        }
    }

    return MakeAtomicShared<TResult>("");
}
