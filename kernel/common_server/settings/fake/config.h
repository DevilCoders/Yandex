#pragma once
#include <kernel/common_server/util/accessor.h>
#include <util/datetime/base.h>
#include <kernel/common_server/settings/abstract/config.h>
#include <kernel/common_server/common/abstract.h>
#include <library/cpp/json/writer/json_value.h>

namespace NCS {
    class TFakeSettingsConfig: public ISettingsConfig {
    private:
        static TFactory::TRegistrator<TFakeSettingsConfig> Registrator;
    protected:
        virtual void DoInit(const TYandexConfig::Section& /*section*/) override {
        }
        virtual void DoToString(IOutputStream& /*os*/) const override {
        }
    public:
        static TString GetTypeName() {
            return "fake";
        }
        virtual TString GetClassName() const override {
            return GetTypeName();
        }

        ISettings::TPtr Construct(const IBaseServer& server) const override;
    };
}
