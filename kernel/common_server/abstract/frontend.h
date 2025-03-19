#pragma once
#include "common.h"
#include <library/cpp/logger/global/global.h>
#include <library/cpp/mediator/messenger.h>
#include <kernel/common_server/notifications/abstract/abstract.h>
#include <kernel/common_server/settings/abstract/abstract.h>
#include <kernel/common_server/geoareas/manager.h>
#include <kernel/common_server/geobase/geobase.h>
#include <kernel/common_server/abstract/external_api.h>

#include <kernel/common_server/library/searchserver/simple/http_status_config.h>
#include <kernel/common_server/library/scheme/operations.h>
#include <kernel/common_server/library/scheme/handler.h>
#include <kernel/common_server/util/types/cache_with_age.h>

#include <util/generic/cast.h>
#include <util/thread/pool.h>

class TItemPermissionContainer;
namespace NExternalAPI {
    class TSender;
}

namespace NRTProc {
    class IVersionedStorage;
}

namespace NCS {
    class IEmulationsManager;
    class IElSignature;
    class IPQConstructionContext;
    class ISnapshotsController;
    class IAbstractCipher;
    class TCertsManger;
    namespace NPropositions {
        class TDBManager;
    }
    namespace NResources {
        class TDBManager;
    }
    namespace NScheme {
        class TConstantsInfoReport;
    }
    namespace NSecret {
        class ISecretsManager;
    }
    namespace NStorage {
        class TDBMigrationsManager;
    }
};

class IAuthModuleConfig;
class TRequestProcessorConfigContainer;
class TLoggerConfig;
class TRTBackgroundManager;
class TS3Client;
class TStartrekClient;
class ILocksManager;
class IPermissionsManager;
class IAuthUsersManager;
class ITagDescriptions;
class TDBTagDescription;
class TTagDescriptionsManager;
class IPersonalDataStorage;
class IRolesManager;
class IUserPermissions;
class IItemPermissions;
class TNotifiersManager;

enum class ELocalization {
    Rus = 0 /* "rus", "ru" */,
    Eng = 1 /* "eng", "en" */,
};

class ILocalization {
private:
    template <class TAction>
    TString ApplyResourcesImpl(const TString& value, const TString& localizationId, const TAction& stringProcessor) const;

protected:
    virtual TMaybe<TString> GetLocalStringImpl(const TString& localizationId, const TString& resourceId) const = 0;
public:
    TString ApplyResources(const TString& value, const TString& localizationId = "rus") const;
    TString ApplyResourcesForJson(const TString& value, const TString& localizationId = "rus") const;
    void ApplyResourcesForJson(NJson::TJsonValue& value, const TString& localizationId = "rus") const;

    virtual ~ILocalization() = default;

    virtual TString GetLocalString(ELocalization localizationId, const TString& resourceId, TMaybe<TString> defaultValue = {}) const final {
        return GetLocalString(ToString(localizationId), resourceId, std::move(defaultValue));
    }

    virtual TString GetLocalString(const TString& localizationId, const TString& resourceId, TMaybe<TString> defaultValue = {}) const final {
        TMaybe<TString> result = GetLocalStringImpl(localizationId, resourceId);
        if (!result) {
            return defaultValue ? *defaultValue : resourceId;
        } else {
            return *result;
        }
    }

    TString FormatPrice(const ELocalization& localizationId, const ui32 price, const std::initializer_list<TString>& units = {}, TStringBuf separator = {}) const;
    TString DistanceFormatKm(const ELocalization localizationId, const double km, bool round = false) const;
    TString FormatDuration(const ELocalization localizationId, const TDuration d, const bool withSeconds = false, const bool allowEmpty = false) const;
    TString FormatInstant(const ELocalization localizationId, const TInstant t) const;
    TString FormatInstantWithYear(const ELocalization localizationId, const TInstant t) const;
    TString FormatTimeOfTheDay(const ELocalization localizationId, const TInstant timestamp, const TDuration shift = TDuration::Hours(3)) const;
    TString FormatBonusRubles(const ELocalization localizationId, const ui32 amount, const bool withAmount = false) const;
    TString FormatFreeWaitTime(const ELocalization localizationId, const TDuration freeWaitTime) const;
    TString FormatIncomingDelegationMessage(const ELocalization localizationId, const TString& delegatorName, const TString& modelName) const;

    TString FormatDelegatedStandartOfferTitle(const ELocalization localizationId, const TString& offerName, const ui32 priceRiding) const;
    TString FormatDelegatedStandartOfferBody(const ELocalization localizationId, const ui32 priceParking, const TDuration freeWaitTime) const;
    TString FormatDelegatedPackOfferTitle(const ELocalization localizationId, const TString& offerName) const;
    TString FormatDelegatedPackOfferBody(const ELocalization localizationId, const ui32 distance, const TDuration duration, const ui32 priceParking, const TDuration freeWaitTime) const;

    TString FormatDelegationBodyWithPackOffer(const ELocalization localizationId, const TString& offerName, const ui32 remainingDistance, const TDuration remainingTime) const;
    TString FormatDelegationBodyWithoutPackOffer(const ELocalization localizationId, const ui32 remainingDistance, const TDuration remainingTime) const;

    TString HoursFormat(const TDuration d, const TString& localizationId = "rus") const;
    TString MinutesFormat(const TDuration d, const TString& localizationId = "rus") const;
    TString DaysFormat(const TDuration d, const TString& localizationId = "rus") const;
};

class TFakeLocalization: public ILocalization {
public:
    virtual TMaybe<TString> GetLocalStringImpl(const TString& /*localizationId*/, const TString& resourceId) const override {
        return resourceId;
    }
};

class TServerStartedMessage: public NMessenger::IMessage {
    RTLINE_ACCEPTOR_DEF(TServerStartedMessage, SequentialTableNames, TSet<TString>);

public:
    TServerStartedMessage() = default;
};

class IBaseServer:
    public INotifiersStorage,
    public virtual NCS::IExternalServicesOperator,
    public NExternalAPI::IRequestCustomizationContext {
private:
    mutable TRWMutex Mutex;
    mutable TMap<TString, TCacheWithAge<TString, NJson::TJsonValue>> Caches;

public:
    IBaseServer();
    ~IBaseServer();
    virtual NCS::NScheme::TConstantsInfoReport BuildInterfaceConstructor(TAtomicSharedPtr<IUserPermissions> permissions) const = 0;
    virtual const NCS::NPropositions::TDBManager* GetPropositionsManager() const = 0;
    virtual TAtomicSharedPtr<NCS::NSecret::ISecretsManager> GetSecretManager(const TString& managerId) const = 0;
    virtual TSet<TString> GetSecretManagerNames() const = 0;
    virtual const NCS::IPQConstructionContext& GetPQConstructionContext() const = 0;
    virtual TVector<NCS::NScheme::THandlerScheme> BuildHandlersScheme(const TSet<TString>& endpoints) const = 0;
    TCacheWithAge<TString, NJson::TJsonValue>& GetCache(const TString& cacheId) const;
    virtual TAtomicSharedPtr<NCS::IElSignature> GetElSignature(const TString& id) const = 0;
    virtual TSet<TString> GetElSignatureIds() const = 0;
    virtual bool IsNoPermissionsMode() const = 0;
    virtual const NCS::ISnapshotsController& GetSnapshotsController() const = 0;
    virtual const TString& GetCType() const = 0;
    virtual const TString& GetServiceName() const = 0;
    virtual const ISettings& GetSettings() const = 0;
    virtual bool HasSettings() const = 0;
    virtual const NCS::NResources::TDBManager* GetResourcesManager() const = 0;
    virtual const ILocksManager& GetLocksManager() const = 0;
    virtual const IGeobaseProvider& GetGeobase() const = 0;
    virtual const TNotifiersManager* GetNotifiersManager() const = 0;
    virtual TAtomicSharedPtr<IFrontendNotifier> GetNotifier(const TString& name) const override = 0;
    virtual TSet<TString> GetNotifierNames() const = 0;
    virtual const TRTBackgroundManager* GetRTBackgroundManager() const = 0;
    virtual TAtomicSharedPtr<NStorage::IDatabase> GetDatabase(const TString& name) const = 0;
    virtual TAtomicSharedPtr<NCS::NStorage::TDBMigrationsManager> GetDBMigrationsManager(const TString& dbName) const = 0;
    virtual TVector<TString> GetDatabaseNames() const = 0;
    virtual IThreadPool* GetRequestsHandler(const TString& handlerName) const = 0;
    virtual TSet<TString> GetServerProcessors() const = 0;
    virtual const IAuthUsersManager& GetAuthUsersManager() const = 0;
    virtual const TTagDescriptionsManager& GetTagDescriptionsManager() const = 0;
    virtual const TS3Client* GetMDSClient() const = 0;
    virtual const THttpStatusManagerConfig& GetHttpStatusManagerConfig() const = 0;

    virtual const NCS::IEmulationsManager* GetEmulationsManager() const = 0;

    virtual TAtomicSharedPtr<NRTProc::IVersionedStorage> GetVersionedStorage(const TString& storageName) const = 0;
    virtual const IAuthModuleConfig* GetAuthModuleInfo(const TString& authModuleName) const = 0;
    virtual TSet<TString> GetAuthModuleIds() const = 0;
    virtual TRequestProcessorConfigContainer GetProcessorInfo(const TString& handlerName) const = 0;
    virtual bool HasProcessorInfo(const TString& handlerName) const = 0;
    virtual const ILocalization& GetLocalization() const = 0;
    virtual const TAtomicSharedPtr<NCS::IAbstractCipher> GetCipherPtr(const TString& cipherName) const = 0;
    virtual const NCS::IAbstractCipher* GetCipher(const TString& cipherName) const = 0;
    virtual TSet<TString> GetCipherNames() const = 0;
    virtual const IGeoAreasManager* GetGeoAreas() const = 0;

    virtual const IRolesManager& GetRolesManager() const = 0;
    virtual TAtomicSharedPtr<IUserPermissions> BuildPermissionFromItems(const TVector<TItemPermissionContainer>& items, const TString& userId) const = 0;
    virtual const IPermissionsManager& GetPermissionsManager() const = 0;
    virtual const IPersonalDataStorage& GetPersonalDataStorage() const = 0;

    virtual void InitConstants(NFrontend::TConstantsInfoReport& cReport, TAtomicSharedPtr<IUserPermissions> permissions) const;
    virtual NJson::TJsonValue GetConstantsReport(const NCS::NScheme::IElement::TReportTraits, const NCS::NScheme::ESchemeFormat) const {
        return NJson::JSON_MAP;
    }

    virtual const NCS::TCertsManger& GetCertsManger() const = 0;

    template <class T>
    const T* GetLocalizationAs() const {
        return dynamic_cast<const T*>(GetLocalization());
    }

    template <class T>
    const T* GetAs() const {
        return dynamic_cast<const T*>(this);
    }

    template <class T>
    const T* GetAsPtrSafe() const {
        const T* result = dynamic_cast<const T*>(this);
        CHECK_WITH_LOG(result);
        return result;
    }

    template <class T>
    const T& GetAsSafe() const {
        return *VerifyDynamicCast<const T*>(this);
    }
};

class ICSOperator {
private:
    const IBaseServer* Server = nullptr;

    const IBaseServer& GetServerImpl() const;

public:
    void SetFrontendServer(const IBaseServer* frontend);

    template <class T = IBaseServer>
    static const T& GetServer() {;
        return Singleton<ICSOperator>()->GetServerImpl().GetAsSafe<T>();
    }

    static bool HasServer() {;
        return !!(Singleton<ICSOperator>()->Server);
    }
};
