#pragma once
#include <library/cpp/yconf/conf.h>
#include <library/cpp/object_factory/object_factory.h>
#include "abstract.h"

namespace NCS {
    class ISettingsConfig {
    protected:
        virtual void DoInit(const TYandexConfig::Section& section) = 0;
        virtual void DoToString(IOutputStream& os) const = 0;
    public:
        using TPtr = TAtomicSharedPtr<ISettingsConfig>;
        using TFactory = NObjectFactory::TObjectFactory<ISettingsConfig, TString>;
        virtual ~ISettingsConfig() = default;
        virtual TString GetClassName() const = 0;
        virtual ISettings::TPtr Construct(const IBaseServer& server) const = 0;
        virtual void Init(const TYandexConfig::Section* section) final;
        virtual void ToString(IOutputStream& os) const final;
    };

    class TSettingsConfigContainer: public TBaseInterfaceContainer<ISettingsConfig> {

    };
}

namespace NFrontend {
    using ISettingsConfig = NCS::ISettingsConfig;
    using TSettingsConfigContainer = NCS::TSettingsConfigContainer;
}
