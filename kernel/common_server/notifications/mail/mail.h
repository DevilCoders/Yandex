#pragma once

#include <kernel/common_server/notifications/abstract/abstract.h>
#include <kernel/common_server/library/metasearch/simple/config.h>
#include <kernel/common_server/library/tvm_services/abstract/abstract.h>

class TMailNotificationsConfig: public IFrontendNotifierConfig {
private:
    using TBase = IFrontendNotifierConfig;
    RTLINE_READONLY_ACCEPTOR_DEF(Account, TString);
    RTLINE_READONLY_ACCEPTOR_DEF(OverridenRecipient, TString);
    RTLINE_READONLY_ACCEPTOR_DEF(AdditionalBccRecipients, TVector<TString>);
    CSA_READONLY(TString, SenderApiName, "mail-backend");

public:
    void DoInit(const TYandexConfig::Section* section) override;
    void DoToString(IOutputStream& os) const override;
    IFrontendNotifier::TPtr Construct() const override;

    Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& info) override;
    NJson::TJsonValue SerializeToJson() const override;
    NFrontend::TScheme GetScheme(const IBaseServer& server) const override;

private:
    static TFactory::TRegistrator<TMailNotificationsConfig> Registrator;
};

// TMailNotifier uses  sender.yandex-team.ru that is based on templates.
// So messages for TMailNotifier must contain tamplate id (sender compaign_id)
// in |Header| and serialized json with template arguments in |Body|.
// Refer to docs here: https://wiki.yandex-team.ru/sender/
//     Direct link: https://github.yandex-team.ru/sendr/sendr/blob/master/docs/transaction-api.md
class TMailNotifier: public IFrontendNotifier {
private:
    using TBase = IFrontendNotifier;
    const TMailNotificationsConfig Config;
    NExternalAPI::TSender::TPtr Sender = nullptr;

public:
    TMailNotifier(const TMailNotificationsConfig& config);
    ~TMailNotifier() override;

    bool DoStart(const NCS::IExternalServicesOperator& /*context*/) override;
    bool DoStop() override;

private:
    TResult::TPtr DoNotify(const TMessage& message, const TContext& context = Default<TContext>()) const override;
};
