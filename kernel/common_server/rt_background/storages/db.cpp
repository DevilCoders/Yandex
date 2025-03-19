#include "db.h"

namespace NBServer {

    TDBRTBackgroundProcessesStorageConfig::TFactory::TRegistrator<TDBRTBackgroundProcessesStorageConfig> TDBRTBackgroundProcessesStorageConfig::Registrator(TDBRTBackgroundProcessesStorageConfig::GetTypeName());

    TStringBuf TDBRTBackgroundProcessesStorage::GetEventObjectId(const TObjectEvent<TRTBackgroundProcessContainer>& ev) const {
        return ev.GetName();
    }

    bool TDBRTBackgroundProcessesStorage::DoRebuildCacheUnsafe() const {
        if (!GetObjects(Objects)) {
            return false;
        }
        return true;
    }

    void TDBRTBackgroundProcessesStorage::AcceptHistoryEventUnsafe(const TObjectEvent<TRTBackgroundProcessContainer>& ev) const {
        if (ev.GetHistoryAction() == EObjectHistoryAction::Remove) {
            Objects.erase(ev.GetName());
        } else {
            auto container = ev.GetDeepCopyUnsafe();
            if (!container) {
                Objects.erase(ev.GetName());
            } else {
                Objects[ev.GetName()] = std::move(container);
            }
        }
    }

    bool TDBRTBackgroundProcessesStorage::GetObjects(TMap<TString, TRTBackgroundProcessContainer>& settings) const {
        settings.clear();
        NStorage::TObjectRecordsSet<TRTBackgroundProcessContainer> records;
        {
            auto session = BuildNativeSession(true);
            TSRSelect select("rt_background_settings", &records);
            if (!session.ExecRequest(select)) {
                ERROR_LOG << "Cannot refresh data for rt_background_manager" << Endl;
                return false;
            }
        }
        for (auto&& i : records) {
            settings.emplace(i.GetName(), std::move(i));
        }
        return true;
    }

    bool TDBRTBackgroundProcessesStorage::RemoveBackground(const TVector<TString>& processIds, const TString& userId) const {
        if (processIds.empty()) {
            return true;
        }
        auto session = BuildNativeSession(false);
        NStorage::TObjectRecordsSet<TRTBackgroundProcessContainer> records;
        TSRDelete srDelete("rt_background_settings", &records);
        srDelete.InitCondition<TSRBinary>("bp_name", processIds);
        if (!session.ExecRequest(srDelete)) {
            return false;
        }
        if (!HistoryManager->AddHistory(records.GetObjects(), userId, EObjectHistoryAction::Remove, session)) {
            return false;
        }

        return session.Commit();
    }

    bool TDBRTBackgroundProcessesStorage::SetBackgroundEnabled(const TSet<TString>& bgNames, const bool enabled, const TString& userId) const {
        if (bgNames.empty()) {
            return true;
        }
        NStorage::TObjectRecordsSet<TRTBackgroundProcessContainer> records;
        TSRUpdate srUpdate("rt_background_settings", &records);
        auto& srMultiCondition = srUpdate.RetCondition<TSRMulti>();
        srMultiCondition.InitNode<TSRBinary>("bp_name", bgNames);
        srMultiCondition.InitNode<TSRBinary>("bp_enabled", !enabled);
        srUpdate.InitUpdate<TSRBinary>("bp_enabled", enabled);

        auto session = BuildNativeSession(false);
        if (!session.ExecRequest(srUpdate)) {
            return false;
        }
        return HistoryManager->AddHistory(records.GetObjects(), userId, EObjectHistoryAction::UpdateData, session) && session.Commit();
    }

    bool TDBRTBackgroundProcessesStorage::UpsertBackgroundSettings(const TRTBackgroundProcessContainer& process, const TString& userId) const {
        if (!process) {
            TFLEventLog::Log("incorrect update request for empty object");
            return false;
        }

        NStorage::TTableRecord trUpdate = process.SerializeToTableRecord();
        NStorage::TTableRecord trCondition;
        trCondition.Set("bp_name", process.GetName());

        auto table = HistoryCacheDatabase->GetTable("rt_background_settings");

        auto session = BuildNativeSession(false);
        NStorage::TObjectRecordsSet<TRTBackgroundProcessContainer> records;
        bool isUpdate = false;
        switch (table->UpsertWithRevision(trUpdate, { "bp_name" }, process.GetRevisionMaybe(), "bp_revision", session.GetTransaction(), &records)) {
            case NStorage::ITableAccessor::EUpdateWithRevisionResult::IncorrectRevision:
                TFLEventLog::Error("incorect_revision");
                return false;
            case NStorage::ITableAccessor::EUpdateWithRevisionResult::Failed:
                TFLEventLog::Error("cannot update");
                return false;
            case NStorage::ITableAccessor::EUpdateWithRevisionResult::Updated:
                isUpdate = true;
            case NStorage::ITableAccessor::EUpdateWithRevisionResult::Inserted:
                break;
        }

        if (!HistoryManager->AddHistory(records.GetObjects(), userId, isUpdate ? EObjectHistoryAction::UpdateData : EObjectHistoryAction::Add, session)) {
            return false;
        }

        return session.Commit();
    }

    bool TDBRTBackgroundProcessesStorage::ForceUpsertBackgroundSettings(const TRTBackgroundProcessContainer& process, const TString& userId) const {
        if (!process) {
            TFLEventLog::Log("incorrect update request for empty object");
            return false;
        }

        NStorage::TTableRecord trUpdate = process.SerializeToTableRecord();
        NStorage::TTableRecord trCondition;
        trCondition.Set("bp_name", process.GetName());
        NStorage::TObjectRecordsSet<TRTBackgroundProcessContainer> records;
        auto table = HistoryCacheDatabase->GetTable("rt_background_settings");
        bool isUpdate;
        trUpdate.ForceSet("bp_revision", "nextval('rt_background_settings_bp_revision_seq')");
        auto session = BuildNativeSession(false);
        if (!table->Upsert(trUpdate, session.GetTransaction(), trCondition, &isUpdate, &records)) {
            TFLEventLog::Error("cannot upsert background settings");
            return false;
        }
        if (records.size() != 1) {
            TFLEventLog::Error("cannot upsert background settings")("reason", "incorrect revision (possible)");
            return false;
        }
        if (!HistoryManager->AddHistory(*records.begin(), userId, isUpdate ? EObjectHistoryAction::UpdateData : EObjectHistoryAction::Add, session)) {
            return false;
        }

        return session.Commit();
    }

    void TDBRTBackgroundProcessesStorageConfig::Init(const TYandexConfig::Section* section) {
        auto children = section->GetAllChildren();
        auto it = children.find("DBStorage");
        if (it == children.end()) {
            DBName = section->GetDirectives().Value("DBName", DBName);
            HistoryConfig.Init(section);
        } else {
            DBName = it->second->GetDirectives().Value("DBName", DBName);
            HistoryConfig.Init(it->second);
        }
    }

    void TDBRTBackgroundProcessesStorageConfig::ToString(IOutputStream& os) const {
        os << "<DBStorage>" << Endl;
        os << "DBName: " << DBName << Endl;
        HistoryConfig.ToString(os);
        os << "</DBStorage>" << Endl;
    }

    THolder<NBServer::IRTBackgroundProcessesStorage> TDBRTBackgroundProcessesStorageConfig::BuildStorage(const IBaseServer& server) const {
        auto db = server.GetDatabase(DBName);
        AssertCorrectConfig(!!db, "incorrect db: %s", DBName.data());
        return MakeHolder<TDBRTBackgroundProcessesStorage>(MakeHolder<THistoryContext>(db), HistoryConfig);
    }

}
