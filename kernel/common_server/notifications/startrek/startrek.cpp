#include "startrek.h"

#include <kernel/common_server/library/json/builder.h>
#include <kernel/common_server/abstract/frontend.h>

TStartrekNotificationsConfig::TFactory::TRegistrator<TStartrekNotificationsConfig> TStartrekNotificationsConfig::Registrator(
        "startrek");

void TStartrekNotificationsConfig::DoInit(const TYandexConfig::Section* section) {
    if (!!section) {
        AssertCorrectConfig(section->GetDirectives().GetValue("Queue", Queue),
                            "Incorrect parameter 'Queue' for startrek notifications");
        SenderApiName = section->GetDirectives().Value<TString>("SenderApiName", SenderApiName);
    }
}

void TStartrekNotificationsConfig::DoToString(IOutputStream& os) const {
    os << "Queue: " << Queue << Endl;
    os << "SenderApiName: " << SenderApiName << Endl;
}


bool TStartrekNotificationsConfig::DeserializeFromJson(const NJson::TJsonValue& info) {
    JREAD_STRING_OPT(info, "queue", Queue);
    JREAD_STRING_OPT(info, "sender_api_name", SenderApiName);
    return TBase::DeserializeFromJson(info);
}

NJson::TJsonValue TStartrekNotificationsConfig::SerializeToJson() const {
    NJson::TJsonValue result = TBase::SerializeToJson();
    JWRITE(result, "queue", Queue);
    JWRITE(result, "sender_api_name", SenderApiName);
    return result;
}

NFrontend::TScheme TStartrekNotificationsConfig::GetScheme(const IBaseServer& server) const {
    NFrontend::TScheme result = TBase::GetScheme(server);
    result.Add<TFSString>("queue").SetDefault(Queue);
    result.Add<TFSString>("sender_api_name").SetDefault(SenderApiName);
    return result;
}

IFrontendNotifier::TPtr TStartrekNotificationsConfig::Construct() const {
    return MakeAtomicShared<TStartrekNotifier>(*this);
}

TStartrekNotifier::TStartrekNotifier(const TStartrekNotificationsConfig& config)
    : IFrontendNotifier(config)
    , Config(config)
{
}

TStartrekNotifier::~TStartrekNotifier() {
}

bool TStartrekNotifier::DoStart(const NCS::IExternalServicesOperator& context) {
    Sender = context.GetSenderPtr(Config.GetSenderApiName());
    if (!Sender) {
        TFLEventLog::Error("Can't start TStartrekNotifier, sender not found")("sender_name", Config.GetSenderApiName());
        return false;
    }
    return true;
}

bool TStartrekNotifier::DoStop() {
    Sender = nullptr;
    return true;
}

IFrontendNotifier::TResult::TPtr
TStartrekNotifier::DoNotify(const TMessage& message, const TContext& /*context*/) const {
    CHECK_WITH_LOG(Sender);
    TStartrekIssueAdaptor issue(message);
    auto resp = Sender->SendRequest(
            NCS::NStartrek::TNewIssueStartrekRequest(issue.GetSummary(), Config.GetQueue(), issue.GetDescription())
            );
    if (!resp.IsSuccess()) {
        return MakeAtomicShared<TResult>("Failed create new issue request");
    }
    return MakeAtomicShared<TResult>("");
}

bool TStartrekNotifier::AddComment(const TMessage& message) const {
    TStartrekCommentAdaptor adaptor(message);
    return AddComment(adaptor.GetIssue(), adaptor.GetComment());
}

bool TStartrekNotifier::AddComment(const TString& issue, const TString& text) const {
    CHECK_WITH_LOG(Sender);
    auto resp = Sender->SendRequest(
            NCS::NStartrek::TAddCommentStartrekRequest(issue, text)
    );
    return resp.IsSuccess();
}
