#pragma once
#include <util/generic/ptr.h>
#include <library/cpp/yconf/conf.h>
#include <util/stream/output.h>
#include <library/cpp/object_factory/object_factory.h>
#include <kernel/common_server/util/accessor.h>
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/http/misc/httpcodes.h>
#include <kernel/common_server/util/json_processing.h>
#include <kernel/common_server/common/scheme.h>
#include <kernel/common_server/abstract/users_contacts.h>
#include <kernel/common_server/api/common.h>
#include <kernel/daemon/config/daemon_config.h>
#include <util/system/hostname.h>
#include <kernel/common_server/util/auto_actualization.h>

namespace NCS {
    class IExternalServicesOperator;
}

class IFrontendNotifier;
class IBaseServer;

namespace NTvmAuth {
    class TTvmClient;
}

class TInternalNotificationMessage: public NMessenger::IMessage {
private:
    CSA_READONLY_DEF(TString, UID);
    CSA_READONLY_DEF(TString, Message);
public:
    TInternalNotificationMessage() = default;
    TInternalNotificationMessage(const TString& uid, const TString& message)
        : UID(uid)
        , Message(message)
    {
    }
};

class IFrontendNotifierConfig {
private:
    TString TypeName;
    CS_ACCESS(IFrontendNotifierConfig, TString, Name, "");
    CSA_READONLY(bool, UseEventLog, true);

protected:
    virtual void DoToString(IOutputStream& os) const = 0;
    virtual void DoInit(const TYandexConfig::Section* section) = 0;
public:
    using TFactory = NObjectFactory::TParametrizedObjectFactory<IFrontendNotifierConfig, TString>;
    using TPtr = TAtomicSharedPtr<IFrontendNotifierConfig>;

    Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& info) {
        JREAD_BOOL_OPT(info, "use_event_log", UseEventLog);
        return true;
    }

    virtual NJson::TJsonValue SerializeToJson() const {
        NJson::TJsonValue result;
        JWRITE(result, "use_event_log", UseEventLog);
        return result;
    }

    virtual NFrontend::TScheme GetScheme(const IBaseServer& /*server*/) const {
        NFrontend::TScheme result;
        result.Add<TFSBoolean>("use_event_log").SetDefault(false);
        return result;
    }

    IFrontendNotifierConfig() = default;
    virtual ~IFrontendNotifierConfig() = default;

    IFrontendNotifierConfig& SetTypeName(const TString& value) {
        TypeName = value;
        return *this;
    }

    const TString& GetTypeName() const {
        return TypeName;
    }

    virtual void Init(const TYandexConfig::Section* section) final {
        UseEventLog = section->GetDirectives().Value("UseEventLog", UseEventLog);
        DoInit(section);
    }

    virtual void InitFromString(const TString& configStr) final {
        TAnyYandexConfig config;
        CHECK_WITH_LOG(config.ParseMemory(configStr.data()));
        Init(config.GetRootSection());
    }

    template<typename TNotifierConfig>
    static TNotifierConfig BuildFromString(const TString& configStr) {
        TNotifierConfig configInstance;
        configInstance.InitFromString(configStr);
        return configInstance;
    }

    virtual void ToString(IOutputStream& os) const final {
        os << "NotificationType: " << TypeName << Endl;
        os << "UseEventLog: " << UseEventLog << Endl;
        DoToString(os);
    }
    virtual TAtomicSharedPtr<IFrontendNotifier> Construct() const = 0;
};

class IFrontendNotifier: public IStartStopProcessImpl<const NCS::IExternalServicesOperator&> {
public:
    IFrontendNotifier(const IFrontendNotifierConfig& config)
        : NotifierName(config.GetName())
        , UseEventLog(config.GetUseEventLog())
    {}

    using TPtr = TAtomicSharedPtr<IFrontendNotifier>;
    using TRecipients = TVector<TUserContacts>;

    class TMessage;
    using TMessages = TVector<TAtomicSharedPtr<TMessage>>;

    class TResult {
        CSA_DEFAULT(TResult, TString, ErrorMessage);
        CSA_DEFAULT(TResult, NJson::TJsonValue, ErrorDetails);
    public:
        using TPtr = TAtomicSharedPtr<TResult>;
        TResult(const TString& error)
            : ErrorMessage(error)
        {
        }

        TResult(const TString& error, const NJson::TJsonValue& errorDetails)
            : ErrorMessage(error)
            , ErrorDetails(errorDetails)
        {
        }

        bool HasErrors() const {
            return !ErrorMessage.empty();
        }

        NJson::TJsonValue SerializeToJson() const {
            NJson::TJsonValue res;
            if (ErrorMessage) {
                res["error_message"] = ErrorMessage;
            }
            if (ErrorDetails.GetType() != NJson::JSON_UNDEFINED) {
                res["error_details"] = ErrorDetails;
            }
            return res;
        }
    };

    class TMessage {
        CS_ACCESS(TMessage, NJson::TJsonValue, AdditionalInfo, NJson::JSON_NULL);
        CSA_DEFAULT(TMessage, TString, Title);
        CSA_DEFAULT(TMessage, TString, MessageHash);
        CSA_DEFAULT(TMessage, TString, Header);
        CSA_DEFAULT(TMessage, TString, Body);

    protected:
        TMessage()
        {
        }

    public:
        TMessage(const TString& body)
            : Body(body)
        {
        }

        TMessage(const TString& header, const TString& body)
            : Header(header)
            , Body(body)
        {
        }

        virtual ~TMessage() = default;

        bool HasAdditionalInfo() const {
            return !AdditionalInfo.IsNull();
        }

        virtual NJson::TJsonValue SerializeToJson() const {
            NJson::TJsonValue json;
            json.InsertValue("AdditionalInfo", AdditionalInfo);
            json.InsertValue("Header", Header);
            json.InsertValue("Body", Body);
            return json;
        }

        template <class T>
        const T* GetAs() const {
            return dynamic_cast<const T*>(this);
        }
    };

    class TContext {
        private:
            CSA_DEFAULT(TContext, TRecipients, Recipients);
            CSA_DEFAULT(TContext, TString, Identity);

            const IBaseServer* Server = nullptr;

        public:
            TContext() = default;
            virtual ~TContext() = default;

            const IBaseServer* GetServer() const {
                return Server;
            }

            TContext& SetServer(const IBaseServer* server) {
                Server = server;
                return *this;
            }
    };

    virtual ~IFrontendNotifier() = default;

    virtual TResult::TPtr Notify(const TMessage& message, const TString& userId = "", const TContext& context = Default<TContext>()) const final;
    virtual void MultiLinesNotify(const TString& commonHeader, const TMessages& reportMessages, const TString& userId = "", const TContext& context = Default<TContext>()) const;
    virtual bool SendDocument(const TMessage& message, const TString& mimeType) const;
    virtual bool SendPhoto(const TMessage& message) const;
    virtual ui32 GetMessageLengthLimit() const {
        return 4096;
    }

    class TAlertInfoCollector: public TStringBuilder {
    private:
        using TBase = TStringBuilder;
        const bool DoAssert = true;
    public:
        TAlertInfoCollector(const bool doAssert)
            : DoAssert(doAssert)
        {
        }

        ~TAlertInfoCollector();
    };

    static TAlertInfoCollector AlertNotification(const bool doAssert = true);

    static void MultiLinesNotify(IFrontendNotifier::TPtr notifier, const TString& commonHeader, const TVector<TString>& reportLines, const TString& userId = "", const TContext& context = Default<TContext>());
    static void MultiLinesNotify(IFrontendNotifier::TPtr notifier, const TString& commonHeader, const TSet<TString>& reportLines, const TString& userId = "", const TContext& context = Default<TContext>());

    template <class T>
    static void MultiLinesNotify(IFrontendNotifier::TPtr notifier, const TString& commonHeader, const TMap<TString, T>& reportInfo, const TString& userId = "", const TContext& context = Default<TContext>()) {
        TVector<TString> linesVector;
        linesVector.reserve(reportInfo.size());
        for (auto&& i : reportInfo) {
            linesVector.emplace_back(i.first + ": " + ::ToString(i.second));
        }
        MultiLinesNotify(notifier, commonHeader, linesVector, userId, context);
    }

    static IFrontendNotifier::TResult::TPtr Notify(IFrontendNotifier::TPtr notifier, const TString& message, const TString& userId = "", const TContext& context = Default<TContext>());
    static bool SendDocument(IFrontendNotifier::TPtr notifier, const IFrontendNotifier::TMessage& message, const TString& mimeType);
    static bool SendPhoto(IFrontendNotifier::TPtr notifier, const IFrontendNotifier::TMessage& message);

    const TString& GetNotifierName() const {
        return NotifierName;
    }

    virtual TMaybe<TString> GetTvmClientName() const {
        return {};
    }
    virtual void SetTvmClient(TAtomicSharedPtr<NTvmAuth::TTvmClient> /*tvmClient*/) {
    }

private:
    virtual TResult::TPtr DoNotify(const TMessage& message, const TContext& context) const = 0;

protected:
    TString NotifierName;
    bool UseEventLog;
};

#define ALERT_NOTIFY IFrontendNotifier::AlertNotification(false) << __LOCATION__ << " " << HostName() << " "

class INotifiersStorage {
public:
    virtual IFrontendNotifier::TPtr GetNotifier(const TString& name) const = 0;
    virtual ~INotifiersStorage() {}
};
