#pragma once

#include "config.h"

#include <kernel/common_server/settings/abstract/abstract.h>
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/api/history/cache.h>
#include <kernel/common_server/api/history/manager.h>

#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/util/types/expected.h>

#include <util/string/join.h>

namespace NCS {

    class TSettingsHistoryManager: public TIndexedAbstractHistoryManager<TSetting> {
    private:
        using TBase = TIndexedAbstractHistoryManager<TSetting>;

    protected:
        using TBase::TBase;
    public:
        TSettingsHistoryManager(const IHistoryContext& context, const THistoryConfig& config)
            : TBase(context, "server_settings_history", config) {
        }
    };

    class TSettingsDB: public TDBCacheWithHistoryOwner<TSettingsHistoryManager, TSetting>, public ISettings {
    private:
        using TBase = TDBCacheWithHistoryOwner<TSettingsHistoryManager, TSetting>;

    private:
        TDuration MaxAge = TDuration::Minutes(1);
        TString Prefix;

    private:
        virtual void AcceptHistoryEventUnsafe(const TObjectEvent<TSetting>& ev) const override;
        virtual bool DoRebuildCacheUnsafe() const override;

        virtual TStringBuf GetEventObjectId(const TObjectEvent<TSetting>& ev) const override {
            return ev.GetKey();
        }

        template <class T>
        bool GetValueImpl(const TString& key, T& result) const {
            const TSet<TString> keys = { key };

            bool found = false;
            const auto action = [&found, &result](const TSetting& setting) {
                found = TryFromString(setting.GetValue(), result);
            };

            if (!ForObjectsList(action, TInstant::Zero(), &keys)) {
                return false;
            }

            return found;
        }

    public:
        TSettingsDB(THolder<IHistoryContext>&& context, const TDBSettingsConfig& config);

        ~TSettingsDB() {
            Stop();
        }

        virtual bool GetAllSettings(TVector<TSetting>& result) const override;

        virtual bool RemoveKeys(const TVector<TString>& keys, const TString& userId) const override;
        virtual bool SetValues(const TVector<TSetting>& values, const TString& userId) const override;

        virtual bool GetHistory(const TInstant since, TVector<TAtomicSharedPtr<TObjectEvent<TSetting>>>& result) const override;

        virtual bool HasValues(const TSet<TString>& keys, TSet<TString>& existKeys) const override;

        virtual bool GetValueStr(const TString& key, TString& result) const override {
            return GetValueImpl(Prefix + key, result);
        }
    };

}
