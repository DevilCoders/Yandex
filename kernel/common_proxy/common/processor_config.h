#pragma once

#include <kernel/daemon/module/module.h>

namespace NCommonProxy {

    class TProcessorConfig : public TPluginConfig<IAbstractModuleConfig> {
    public:
        typedef NObjectFactory::TParametrizedObjectFactory<TProcessorConfig, TString, TString> TFactory;

    public:
        TProcessorConfig(const TString& type)
            : Type(type)
        {}

        const TString& GetType() const;
        ui32 GetThreads() const;
        ui32 GetMaxQueueSize() const;
        bool IsIgnoredSignal(const TString& signal) const;

    protected:
        virtual void DoToString(IOutputStream& so) const override;
        virtual void DoInit(const TYandexConfig::Section& componentSection) override;
        virtual bool DoCheck() const override;

    private:
        ui32 Threads = 0;
        ui32 MaxQueueSize = 0;
        const TString Type;
        THashSet<TString> IgnoredSignals;
    };

    typedef TTypedPluginConfigs<TProcessorConfig> TProcessorsConfigs;

}
