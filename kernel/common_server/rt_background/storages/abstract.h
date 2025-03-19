#pragma once
#include <kernel/common_server/rt_background/settings.h>

namespace NBServer {
    class IRTBackgroundProcessesStorage {
    public:
        virtual ~IRTBackgroundProcessesStorage() = default;
        virtual TMaybe<TRTBackgroundProcessContainer> GetObject(const TString& processId) const final;
        virtual bool GetObjects(TMap<TString, TRTBackgroundProcessContainer>& objects) const = 0;
        virtual bool RemoveBackground(const TVector<TString>& processIds, const TString& userId) const = 0;
        virtual bool SetBackgroundEnabled(const TSet<TString>& bgNames, const bool enabled, const TString& userId) const = 0;
        virtual bool UpsertBackgroundSettings(const TRTBackgroundProcessContainer& process, const TString& userId) const = 0;
        virtual bool ForceUpsertBackgroundSettings(const TRTBackgroundProcessContainer& process, const TString& userId) const = 0;
        virtual bool StartStorage() = 0;
        virtual bool StopStorage() = 0;
    };

    class IRTBackgroundStorageConfig {
    public:
        using TFactory = NObjectFactory::TObjectFactory<IRTBackgroundStorageConfig, TString>;
        using TPtr = TAtomicSharedPtr<IRTBackgroundStorageConfig>;
        virtual ~IRTBackgroundStorageConfig() = default;
        virtual TString GetClassName() const = 0;
        virtual void Init(const TYandexConfig::Section* section) = 0;
        virtual void ToString(IOutputStream& os) const = 0;
        virtual THolder<IRTBackgroundProcessesStorage> BuildStorage(const IBaseServer& server) const = 0;
    };

    class TDefaultInterfaceRTProcessesContainerConfiguration {
    public:
        static TString GetDefaultClassName() {
            return "db";
        }
        static TString GetClassNameField() {
            return "class_name";
        }
        static TString GetConfigClassNameField() {
            return "StorageType";
        }
        static TString GetSpecialSectionForType(const TString& /*className*/) {
            return "";
        }
    };

    class TRTBackgroundStorageConfigContainer: public TBaseInterfaceContainer<IRTBackgroundStorageConfig, TDefaultInterfaceRTProcessesContainerConfiguration> {

    };

}
