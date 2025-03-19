#include "telegram.h"
#include <kernel/common_server/util/network/neh_request.h>
#include <kernel/common_server/util/network/neh.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <util/string/builder.h>
#include <library/cpp/mediator/global_notifications/system_status.h>
#include <library/cpp/html/escape/escape.h>
#include <util/string/subst.h>
#include <kernel/common_server/library/logging/events.h>

void TTelegramNotifierImpl::Start() {
    CHECK_WITH_LOG(!Agent);
    AD = MakeAtomicShared<TAsyncDelivery>();
    AD->Start(1, 8);
    Agent = MakeHolder<NNeh::THttpClient>(AD);
    Agent->RegisterSource("telegram", "api.telegram.org", 443, Config.GetReaskConfig(), true);
}

class TTelegramRequestCallback: public NNeh::THttpAsyncReport::ICallback {
private:
    const TString OriginalRequest;
public:
    TTelegramRequestCallback(const TString& original)
        : OriginalRequest(original)
    {

    }
    void OnResponse(const TVector<NNeh::THttpAsyncReport>& reports) override {
        for (auto&& i : reports) {
            if (i.GetHttpCode() != 200) {
                TFLEventLog::Log("Report failure",TLOG_ERR)
                    ("request", OriginalRequest)
                    ("source info", i.GetSourceInfo())
                    ("code", i.GetHttpCode())
                    ("report", (i.HasReport() ? i.GetReportSafe() : " NO_REPORT"));
            }
        }

    }
};

IFrontendNotifier::TResult::TPtr TTelegramNotifierImpl::Notify(const IFrontendNotifier::TMessage& message, const TString& chatId) const {
    if (!Agent) {
        TFLEventLog::Log("Incorrect agent for notifier", TLOG_ERR);
        return nullptr;
    }
    NNeh::THttpRequest request;
    TString report = message.GetHeader();
    if (!!report) {
        report += ": ";
    }
    report += message.GetBody();
    report = NHtml::EscapeText(report);
    request.SetCgiData("chat_id=" + chatId + "&parse_mode=HTML&text=" + CGIEscapeRet(report));
    request.SetUri("/" + Config.GetBotId() + "/sendMessage");
    auto callback = MakeHolder<TTelegramRequestCallback>(request.GetCgiData());
    Agent->Send(request, Now() + Config.GetReaskConfig().GetGlobalTimeout(), std::move(callback));
    return nullptr;
}

bool TTelegramNotifierImpl::SendDocument(const IFrontendNotifier::TMessage& message, const TString& mimeType, const TString& chatId) const {
    NNeh::THttpRequest request;
    request.SetUri("/" + Config.GetBotId() + "/sendDocument");
    return SendAttachment(request, message, EAttachmentType::Document, mimeType, chatId);
}

bool TTelegramNotifierImpl::SendPhoto(const IFrontendNotifier::TMessage& message, const TString& chatId) const {
    NNeh::THttpRequest request;
    request.SetUri("/" + Config.GetBotId() + "/sendPhoto");
    return SendAttachment(request, message, EAttachmentType::Photo, "image/jpeg", chatId);
}

bool TTelegramNotifierImpl::SendAttachment(const NNeh::THttpRequest& simpleRequest, const IFrontendNotifier::TMessage& message, EAttachmentType type, const TString& mimeType, const TString& chatId) const {
    if (!Agent) {
        TFLEventLog::Log("Incorrect agent for notifier", TLOG_ERR);
        return false;
    }
    NNeh::THttpRequest request = simpleRequest;
    request.SetCgiData("chat_id=" + chatId + "&caption=" + message.GetHeader() + ":" + message.GetAdditionalInfo().GetStringRobust());
    request.SetRequestType("POST");

    TString boundary = "--Asrf456BGe4h";
    request.AddHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

    TString data = TStringBuilder()
        << "--" << boundary << "\r\n"
        << "Content-Disposition: form-data; name=\"" + ToString(type) + "\"; filename=\"" + message.GetTitle() + "\"\r\n"
        << "Content-Type: " << mimeType << "\r\n"
        << "Content-Length: " << message.GetBody().size() << "\r\n\r\n"
        << message.GetBody() << "\r\n"
        << "--" << boundary << "--\r\n";

    request.SetPostData(data);

    auto tgResult = Agent->SendMessageSync(request, Now() + TDuration::Seconds(60));
    if (tgResult.Code() != 200) {
        TFLEventLog::Log("send attachment", TLOG_DEBUG)
            ("code", tgResult.Code())
            ("content", tgResult.Content())
            ("error message", tgResult.ErrorMessage());
        return false;
    }
    return true;
}

TTelegramNotifierImpl::TTelegramNotifierImpl(const TTelegramBotConfig& config)
    : Config(config)
{
}

void TTelegramNotifierImpl::Stop() {
    if (!!AD) {
        AD->Stop();
    }
}

TTelegramNotificationsConfig::TFactory::TRegistrator<TTelegramNotificationsConfig> TTelegramNotificationsConfig::Registrator("telegram");


IFrontendNotifier::TPtr TTelegramNotificationsConfig::Construct() const {
    return MakeAtomicShared<TTelegramNotifier>(*this);
}

void TTelegramChatConfig::Init(const TYandexConfig::Section* section) {
    AssertCorrectConfig(section->GetDirectives().GetValue("ChatId", ChatId), "Incorrect parameter 'ChatId' for telegram notification");
}

void TTelegramBotConfig::Init(const TYandexConfig::Section* section) {
    auto children = section->GetAllChildren();
    auto it = children.find("ReaskConfig");
    if (it == children.end()) {
        ReaskConfig.SetGlobalTimeout(TDuration::Seconds(14)).SetTasksCheckInterval(TDuration::Seconds(7)).SetMaxAttempts(2);
    } else {
        ReaskConfig.InitFromSection(it->second);
    }
    AssertCorrectConfig(section->GetDirectives().GetValue("BotId", BotId), "Incorrect parameter 'BotId' for telegram notification");
}

void TTelegramBotConfig::ToString(IOutputStream& os) const {
    os << "BotId: " << BotId << Endl;
    os << "<ReaskPolicy>" << Endl;
    ReaskConfig.ToString(os);
    os << "</ReaskPolicy>" << Endl;
}

TTelegramNotifier::~TTelegramNotifier() {
}
