#pragma once
#include <kernel/common_server/settings/abstract/config.h>

namespace NCS {
    class TSettingsPackConfig: public ISettingsConfig {
    private:
        static ISettingsConfig::TFactory::TRegistrator<TSettingsPackConfig> Registrator;
        TMap<TString, TSettingsConfigContainer> Configs;
    protected:
        virtual void DoInit(const TYandexConfig::Section& section) override;
        virtual void DoToString(IOutputStream& os) const override;
    public:
        static TString GetTypeName() {
            return "settings-pack";
        }
        virtual TString GetClassName() const override {
            return GetTypeName();
        }

        virtual ISettings::TPtr Construct(const IBaseServer& server) const override;
    };
}
