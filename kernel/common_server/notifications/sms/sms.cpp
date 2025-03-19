#include "sms.h"

#include <library/cpp/string_utils/base64/base64.h>
#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/abstract/frontend.h>
#include <library/cpp/digest/md5/md5.h>

#include "request.h"

TSMSNotificationsConfig::TFactory::TRegistrator<TSMSNotificationsConfig> TSMSNotificationsConfig::Registrator("sms");

void TSMSNotificationsConfig::DoInit(const TYandexConfig::Section* section) {
    RequireUid = section->GetDirectives().Value<bool>("RequireUid", RequireUid);
    AssertCorrectConfig(section->GetDirectives().GetValue("Sender", Sender),
                        "Incorrect parameter 'Sender' for sms notifications");
    AssertCorrectConfig(section->GetDirectives().GetValue("Route", Route),
                        "Incorrect parameter 'Route' for sms notifications");
    SenderApiName = section->GetDirectives().Value<TString>("SenderApiName", SenderApiName);

    TVector<TString> whiteList;
    section->GetDirectives().TryFillArray("WhiteList", whiteList);
    WhiteList.clear();
    for (auto& item : whiteList) {
        WhiteList.insert(item);
    }
}

void TSMSNotificationsConfig::DoToString(IOutputStream& os) const {
    os << "Sender: " << Sender << Endl;
    os << "Route: " << Route << Endl;
    os << "RequireUid: " << RequireUid << Endl;
    os << "WhiteList: " << JoinSeq(", ", WhiteList) << Endl;
    os << "SenderApiName: " << SenderApiName << Endl;
}

IFrontendNotifier::TPtr TSMSNotificationsConfig::Construct() const {
    return MakeAtomicShared<TSMSNotifier>(*this);
}

bool TSMSNotificationsConfig::DeserializeFromJson(const NJson::TJsonValue& info) {
    JREAD_STRING_OPT(info, "sender", Sender);
    JREAD_STRING_OPT(info, "route", Route);
    JREAD_BOOL_OPT(info, "require_uid", RequireUid);
    JREAD_CONTAINER(info, "white_list", WhiteList);
    JREAD_STRING_OPT(info, "sender_api_name", SenderApiName);
    return TBase::DeserializeFromJson(info);
}

NJson::TJsonValue TSMSNotificationsConfig::SerializeToJson() const {
    NJson::TJsonValue result = TBase::SerializeToJson();
    JWRITE(result, "sender", Sender);
    JWRITE(result, "route", Route);
    JWRITE(result, "require_uid", RequireUid);
    TJsonProcessor::WriteContainerArray(result, "white_list", WhiteList, false);
    JWRITE(result, "sender_api_name", SenderApiName);
    return result;
}

NFrontend::TScheme TSMSNotificationsConfig::GetScheme(const IBaseServer& server) const {
    NFrontend::TScheme result = TBase::GetScheme(server);
    result.Add<TFSString>("sender").SetDefault(Sender);
    result.Add<TFSString>("route").SetDefault(Route);
    result.Add<TFSBoolean>("require_uid").SetDefault(RequireUid);
    result.Add<TFSArray>("white_list").SetElement<TFSString>();
    result.Add<TFSString>("sender_api_name").SetDefault(SenderApiName);
    return result;
}

TSMSNotifier::TSMSNotifier(const TSMSNotificationsConfig& config)
    : IFrontendNotifier(config)
    , Config(config)
{
}

TSMSNotifier::~TSMSNotifier() {
}

bool TSMSNotifier::DoStart(const NCS::IExternalServicesOperator& context) {
    Sender = context.GetSenderPtr(Config.GetSenderApiName());
    return true;
}

bool TSMSNotifier::DoStop() {
    Sender = nullptr;
    return true;
}

IFrontendNotifier::TResult::TPtr TSMSNotifier::DoNotify(const TMessage& message, const TContext& context) const {
    CHECK_WITH_LOG(Sender);

    auto grLog = TFLRecords::StartContext()("text", message.GetBody());

    TMap<TString, TSmsRequest> requests;
    for (auto&& r : context.GetRecipients()) {
        if (!Config.GetWhiteList().empty() &&
            !Config.GetWhiteList().contains(r.GetPhone())) {
            continue;
        }

        if (r.GetPhone().empty()) {
            TFLEventLog::Log("Missing phone number", TLOG_ERR)("user_id", r.GetUserId());
            return nullptr;
        }

        TSmsRequest request(Config.GetSender(), message.GetBody());
        if (Config.GetRequireUid()) {
            if (r.GetPassportUid().empty()) {
                TFLEventLog::Log("Missing required uid", TLOG_ERR)("user_id", r.GetUserId());
                return nullptr;
            }
            request.SetUID(r.GetPassportUid());
        }

        const TString key = r.GetPhone() + ":" +  r.GetPassportUid();
        TString identity;
        if (!context.GetIdentity().empty()) {
            identity = MD5::Calc(key + ":" + context.GetIdentity());
        } else {
            identity = MD5::Calc(key + ":" + message.GetBody());
        }

        request.SetPhone(r.GetPhone());
        request.SetIdentity(identity);
        requests.emplace(key, request);
    }

    auto responses = Sender->SendPack(requests);
    for (const auto& [key, resp] : responses) {
        if (!resp.IsSuccess()) {
            return MakeAtomicShared<TResult>("Failed request");
        }
    }

    return MakeAtomicShared<TResult>("");
}
