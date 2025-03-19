#pragma once
#include <library/cpp/mediator/messenger.h>
#include <kernel/common_server/util/accessor.h>
#include <util/datetime/base.h>
#include <kernel/common_server/settings/abstract/abstract.h>
#include <library/cpp/json/writer/json_value.h>

namespace NCS {
    class TFakeSettings: public ISettings {
    private:
        using TBase = ISettings;
    public:
        using TBase::TBase;

        virtual bool HasValues(const TSet<TString>& /*keys*/, TSet<TString>& existKeys) const override {
            existKeys.clear();
            return true;
        }
        virtual bool GetValueStr(const TString& /*key*/, TString& /*result*/) const override {
            return false;
        }
        virtual bool RemoveKeys(const TVector<TString>& /*keys*/, const TString& /*userId*/) const override {
            return true;
        }
        virtual bool SetValues(const TVector<TSetting>& /*values*/, const TString& /*userId*/) const override {
            return false;
        }
        virtual bool GetHistory(const TInstant /*since*/, TVector<TAtomicSharedPtr<TObjectEvent<TSetting>>>& /*result*/) const override {
            return true;
        }
        virtual bool GetAllSettings(TVector<TSetting>& /*result*/) const override {
            return true;
        }
    };
}
