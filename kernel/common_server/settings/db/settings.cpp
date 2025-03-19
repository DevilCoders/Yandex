#include "settings.h"

namespace NCS {

    bool TSettingsDB::SetValues(const TVector<TSetting>& values, const TString& userId) const {
        auto gLogging = TFLRecords::StartContext().Method("TSettingsDB::SetValues");
        NStorage::ITransaction::TPtr transaction = HistoryCacheDatabase->CreateTransaction(false);
        NStorage::ITableAccessor::TPtr table = HistoryCacheDatabase->GetTable("server_settings");
        NCS::TEntitySession session(transaction);
        for (auto&& i : values) {
            TSetting setting(Prefix + i.GetKey(), i.GetValue());
            const NStorage::TTableRecord record = setting.SerializeToTableRecord();
            const NStorage::TTableRecord unique = setting.SerializeUniqueToTableRecord();
            bool isUpdate;
            if (!table->Upsert(record, transaction, unique, &isUpdate)) {
                return false;
            }
            if (!HistoryManager->AddHistory(setting, userId, isUpdate ? EObjectHistoryAction::UpdateData : EObjectHistoryAction::Add, session)) {
                return false;
            }
        }
        if (!transaction->Commit()) {
            return false;
        }
        return true;
    }

    bool TSettingsDB::GetHistory(const TInstant since, TVector<TAtomicSharedPtr<TObjectEvent<TSetting>>>& result) const {
        TVector<TAtomicSharedPtr<TObjectEvent<TSetting>>> resultForAll;

        if (!GetHistoryManager().GetEventsAll(since, resultForAll, TInstant::Zero())) {
            return false;
        }
        for (auto&& i : resultForAll) {
            if (i->GetKey().StartsWith(Prefix)) {
                result.emplace_back(i);
            }
        }
        return true;
    }

    bool TSettingsDB::HasValues(const TSet<TString>& keys, TSet<TString>& existKeys) const {
        existKeys.clear();
        if (keys.empty())
            return true;

        const auto action = [&keys, &existKeys](const TSetting& setting) {
            if (keys.contains(setting.GetKey())) {
                existKeys.emplace(setting.GetKey());
            }
        };

        return ForObjectsList(action, TInstant::Zero(), &keys);
    }

    TSettingsDB::TSettingsDB(THolder<IHistoryContext>&& context, const TDBSettingsConfig& config)
        : TBase(std::move(context), config.GetHistoryConfig())
        , ISettings(config)
        , Prefix(config.GetPrefix())
    {
        CHECK_WITH_LOG(Start()) << "cannot start settings db" << Endl;
    }

    bool TSettingsDB::GetAllSettings(TVector<TSetting>& result) const {
        const auto action = [this, &result](const TSetting& setting) {
            if (!Prefix) {
                result.emplace_back(setting);
            } else if (setting.GetKey().StartsWith(Prefix)) {
                result.emplace_back(setting);
                result.back().SetKey(setting.GetKey().substr(Prefix.size()));
            }
        };
        return ForObjectsList(action, TInstant::Zero());
    }

    bool TSettingsDB::RemoveKeys(const TVector<TString>& keys, const TString& userId) const {
        auto gLogging = TFLRecords::StartContext().Method("TSettingsDB::RemoveKeys");
        if (keys.empty())
            return true;
        NCS::NStorage::TObjectRecordsSet<TSetting> records;
        TSRDelete srDelete("server_settings", &records);
        NStorage::ITransaction::TPtr transaction = HistoryCacheDatabase->CreateTransaction(false);
        TVector<TString> keysPrefixed = keys;
        for (auto&& i : keysPrefixed) {
            i = Prefix + i;
        }
        srDelete.InitCondition<TSRBinary>("setting_key", keysPrefixed);
        NCS::TEntitySession session(transaction);
        if (!session.ExecRequest(srDelete)) {
            return false;
        }
        if (!HistoryManager->AddHistory(records.GetObjects(), userId, EObjectHistoryAction::Remove, session)) {
            return false;
        }
        if (!transaction->Commit()) {
            return false;
        }
        return true;
    }

    void TSettingsDB::AcceptHistoryEventUnsafe(const TObjectEvent<TSetting>& ev) const {
        if (ev.GetHistoryAction() == EObjectHistoryAction::Remove) {
            Objects.erase(ev.GetKey());
        } else {
            Objects[ev.GetKey()] = ev;
        }
    }

    bool TSettingsDB::DoRebuildCacheUnsafe() const {
        NCS::NStorage::TRecordsSetWT records;
        auto transaction = HistoryCacheDatabase->CreateTransaction(true);
        TSRSelect select("server_settings", &records);
        if (!transaction->ExecRequest(select)->IsSucceed()) {
            return false;
        }

        TVector<TSetting> settings;
        for (auto&& i : records) {
            TSetting setting;
            if (setting.DeserializeFromTableRecord(i, nullptr)) {
                Objects.emplace(setting.GetKey(), std::move(setting));
            }
        }
        return true;
    }

}
