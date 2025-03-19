#pragma once

#include <kernel/common_server/startrek/issue_requests.h>
#include <kernel/common_server/startrek/comment_requests.h>
#include <kernel/common_server/notifications/abstract/abstract.h>
#include <kernel/common_server/library/tvm_services/abstract/abstract.h>

#include <kernel/daemon/config/daemon_config.h>

#include <library/cpp/json/json_reader.h>

class TStartrekNotificationsConfig: public IFrontendNotifierConfig {
private:
    using TBase = IFrontendNotifierConfig;
    CSA_READONLY_DEF(TString, Queue);
    CSA_READONLY(TString, SenderApiName, "startrek-backend");
public:
    void DoInit(const TYandexConfig::Section* section) override;
    void DoToString(IOutputStream& os) const override;
    IFrontendNotifier::TPtr Construct() const override;

    virtual bool DeserializeFromJson(const NJson::TJsonValue& info) override;
    virtual NJson::TJsonValue SerializeToJson() const override;
    virtual NFrontend::TScheme GetScheme(const IBaseServer& server) const override;

private:
    static TFactory::TRegistrator<TStartrekNotificationsConfig> Registrator;
};

class TStartrekNotifier: public IFrontendNotifier {
private:
    using TBase = IFrontendNotifier;
    const TStartrekNotificationsConfig Config;
    NExternalAPI::TSender::TPtr Sender;

public:
    TStartrekNotifier(const TStartrekNotificationsConfig& config);

    virtual ~TStartrekNotifier() override;

    bool DoStart(const NCS::IExternalServicesOperator& /*context*/) override;
    bool DoStop() override;

    bool AddComment(const TMessage& message) const;
    bool AddComment(const TString& issue, const TString& message) const;
private:
    TResult::TPtr DoNotify(const TMessage& message, const TContext& context = Default<TContext>()) const override;

public:
    class TStartrekCommentAdaptor {
        const TMessage& Message;
    public:
        TStartrekCommentAdaptor(const TMessage& message)
                : Message(message)
        {}

        const TString& GetIssue() const {
            return Message.GetHeader();
        }

        const TString& GetComment() const {
            return Message.GetBody();
        }
    };

    class TStartrekIssueAdaptor {
        const TMessage& Message;
    public:
        TStartrekIssueAdaptor(const TMessage& message)
                : Message(message)
        {}

        const TString& GetSummary() {
            return Message.GetHeader();
        }

        const TString& GetDescription() {
            return Message.GetBody();
        }
    };
};
