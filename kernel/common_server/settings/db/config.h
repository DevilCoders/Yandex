#pragma once

#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/util/types/expected.h>

#include <util/string/join.h>
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/api/history/config.h>
#include <kernel/common_server/settings/abstract/config.h>

namespace NCS {
    class TDBSettingsConfig: public ISettingsConfig {
    private:
        static TFactory::TRegistrator<TDBSettingsConfig> Registrator;
        CSA_READONLY_DEF(THistoryConfig, HistoryConfig);
        CSA_READONLY_DEF(TString, DBName);
        CSA_READONLY_DEF(TString, Prefix);
    protected:
        virtual void DoInit(const TYandexConfig::Section& section) override;
        virtual void DoToString(IOutputStream& os) const override;
    public:
        static TString GetTypeName() {
            return "db";
        }

        virtual TString GetClassName() const override {
            return GetTypeName();
        }
        virtual ISettings::TPtr Construct(const IBaseServer& server) const override;

        TDBSettingsConfig() = default;
    };

}
