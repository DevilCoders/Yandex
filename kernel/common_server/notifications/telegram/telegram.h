#pragma once
#include <util/generic/ptr.h>
#include <library/cpp/yconf/conf.h>
#include <util/stream/output.h>
#include <kernel/common_server/notifications/abstract/abstract.h>
#include <kernel/common_server/library/metasearch/simple/config.h>
#include <kernel/common_server/library/async_proxy/async_delivery.h>
#include <kernel/common_server/util/network/neh.h>
#include <kernel/common_server/util/accessor.h>

namespace NNeh {
    class THttpClient;
}

class TAsyncDelivery;

class TReaskConfigOperator {
public:
    Y_WARN_UNUSED_RESULT static bool DeserializeFromJson(NSimpleMeta::TConfig& config, const NJson::TJsonValue& info) {
        JREAD_INT_OPT(info, "max_attempts", config.MutableMaxAttempts());
        JREAD_DURATION_OPT(info, "global_timeout", config.MutableGlobalTimeout());
        JREAD_DURATION_OPT(info, "tasks_check_interval", config.MutableTasksCheckInterval());
        return true;
    }
    static NJson::TJsonValue SerializeToJson(const NSimpleMeta::TConfig& config) {
        NJson::TJsonValue result;
        JWRITE(result, "max_attempts", config.GetMaxAttempts());
        TJsonProcessor::WriteDurationString(result, "global_timeout", config.GetGlobalTimeout());
        TJsonProcessor::WriteDurationString(result, "tasks_check_interval", config.GetTasksCheckInterval());
        return result;
    }
    static NFrontend::TScheme GetScheme() {
        NFrontend::TScheme result;
        result.Add<TFSNumeric>("max_attempts").SetDefault(2);
        result.Add<TFSDuration>("global_timeout").SetDefault(TDuration::MilliSeconds(1000));
        result.Add<TFSDuration>("tasks_check_interval").SetDefault(TDuration::MilliSeconds(500));
        return result;
    }
};

class TTelegramBotConfig {
private:
    TString BotId;
    RTLINE_READONLY_ACCEPTOR_DEF(ReaskConfig, NSimpleMeta::TConfig);
public:
    void Init(const TYandexConfig::Section* section);

    void ToString(IOutputStream& os) const;

    const TString& GetBotId() const {
        return BotId;
    }

    Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& info) {
        JREAD_STRING_OPT(info, "bot_id", BotId);
        if (info.Has("reask_config") && !TReaskConfigOperator::DeserializeFromJson(ReaskConfig, info["reask_config"])) {
            return false;
        }
        return true;
    }

    NJson::TJsonValue SerializeToJson() const {
        NJson::TJsonValue result;
        result.InsertValue("bot_id", BotId);
        result.InsertValue("reask_config", TReaskConfigOperator::SerializeToJson(ReaskConfig));
        return result;
    }

    NFrontend::TScheme GetScheme(const IBaseServer& /*server*/) const {
        NFrontend::TScheme result;
        result.Add<TFSString>("bot_id");
        result.Add<TFSStructure>("reask_config").SetStructure(TReaskConfigOperator::GetScheme());
        return result;
    }
};

class TTelegramChatConfig {
private:
    TString ChatId;
public:
    void Init(const TYandexConfig::Section* section);

    void ToString(IOutputStream& os) const {
        os << "ChatId: " << ChatId << Endl;
    }

    const TString& GetChatId() const {
        return ChatId;
    }

    Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& info) {
        JREAD_STRING_OPT(info, "chat_id", ChatId);
        return true;
    }

    NJson::TJsonValue SerializeToJson() const {
        NJson::TJsonValue result;
        result.InsertValue("chat_id", ChatId);
        return result;
    }

    NFrontend::TScheme GetScheme(const IBaseServer& /*server*/) const {
        NFrontend::TScheme result;
        result.Add<TFSString>("chat_id");
        return result;
    }
};

class TTelegramNotificationsConfig: public IFrontendNotifierConfig {
private:
    using TBase = IFrontendNotifierConfig;
    TTelegramBotConfig Bot;
    TTelegramChatConfig Chat;
    static TFactory::TRegistrator<TTelegramNotificationsConfig> Registrator;

public:
    virtual IFrontendNotifier::TPtr Construct() const override;

    const TTelegramBotConfig& GetBot() const {
        return Bot;
    }

    const TTelegramChatConfig& GetChat() const {
        return Chat;
    }

    Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& info) override {
        if (!info.Has("bot") || !info.Has("chat")) {
            return false;
        }
        if (!Bot.DeserializeFromJson(info["bot"])) {
            return false;
        }
        if (!Chat.DeserializeFromJson(info["chat"])) {
            return false;
        }
        return TBase::DeserializeFromJson(info);
    }

    virtual NJson::TJsonValue SerializeToJson() const override {
        NJson::TJsonValue result = TBase::SerializeToJson();
        result.InsertValue("bot", Bot.SerializeToJson());
        result.InsertValue("chat", Chat.SerializeToJson());
        return result;
    }

    virtual NFrontend::TScheme GetScheme(const IBaseServer& server) const override {
        NFrontend::TScheme result = TBase::GetScheme(server);
        result.Add<TFSStructure>("bot").SetStructure(Bot.GetScheme(server));
        result.Add<TFSStructure>("chat").SetStructure(Chat.GetScheme(server));
        return result;
    }
private:
    virtual void DoInit(const TYandexConfig::Section* section) override {
        Bot.Init(section);
        Chat.Init(section);
    }

    virtual void DoToString(IOutputStream& os) const override {
        Bot.ToString(os);
        Chat.ToString(os);
    }
};

enum class EAttachmentType {
    Document /* "document" */,
    Photo /* "photo" */,
};

class TTelegramNotifierImpl: public TNonCopyable {
protected:
    TAtomicSharedPtr<TAsyncDelivery> AD;
    THolder<NNeh::THttpClient> Agent;
    const TTelegramBotConfig Config;

public:
    IFrontendNotifier::TResult::TPtr Notify(const IFrontendNotifier::TMessage& message, const TString& chatId) const;
    bool SendDocument(const IFrontendNotifier::TMessage& message, const TString& mimeType, const TString& chatId) const;
    bool SendPhoto(const IFrontendNotifier::TMessage& message, const TString& chatId) const;

    TTelegramNotifierImpl(const TTelegramBotConfig& config);

    void Start();

    void Stop();

private:
    bool SendAttachment(const NNeh::THttpRequest& simpleRequest, const IFrontendNotifier::TMessage& message, EAttachmentType type, const TString& mimeType, const TString& chatId) const;
};

class TTelegramNotifier: public TTelegramNotifierImpl, public IFrontendNotifier {
    using TBase = IFrontendNotifier;
protected:
    const TTelegramNotificationsConfig Config;
public:
    TTelegramNotifier(const TTelegramNotificationsConfig& config)
        : TTelegramNotifierImpl(config.GetBot())
        , IFrontendNotifier(config)
        , Config(config)
    {}

    virtual ~TTelegramNotifier();

    virtual bool DoStart(const NCS::IExternalServicesOperator& /*context*/) override {
        TTelegramNotifierImpl::Start();
        return true;
    }

    virtual bool SendDocument(const TMessage& message, const TString& mimeType) const override {
        return TTelegramNotifierImpl::SendDocument(message, mimeType, Config.GetChat().GetChatId());
    }

    virtual bool SendPhoto(const TMessage& message) const override {
        return TTelegramNotifierImpl::SendPhoto(message, Config.GetChat().GetChatId());
    }

    virtual bool DoStop() override {
        TTelegramNotifierImpl::Stop();
        return true;
    }

private:
    virtual TResult::TPtr DoNotify(const TMessage& message, const TContext& /*context*/ = Default<TContext>()) const override {
        return TTelegramNotifierImpl::Notify(message, Config.GetChat().GetChatId());
    }
};
