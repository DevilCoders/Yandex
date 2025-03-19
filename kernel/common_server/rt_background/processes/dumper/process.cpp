#include "process.h"
#include "yt_dumper.h"

#include <library/cpp/yson/node/node_io.h>

#include <kernel/common_server/util/json_processing.h>
#include <kernel/common_server/rt_background/state.h>
#include <kernel/common_server/library/unistat/cache.h>
#include <kernel/common_server/library/yt/common/writer.h>

IRTRegularBackgroundProcess::TFactory::TRegistrator<TRTDumperWatcher> TRTDumperWatcher::Registrator(TRTDumperWatcher::GetTypeName());
TRTDumperState::TFactory::TRegistrator<TRTDumperState> TRTDumperState::Registrator(TRTDumperWatcher::GetTypeName());

IRTRegularBackgroundProcess::TFactory::TRegistrator<TRTDumperWatcher> TRTDumperWatcher::RegistratorForBackwardCompatibility("dumper_watcher");
TRTDumperState::TFactory::TRegistrator<TRTDumperState> TRTDumperState::RegistratorForBackwardCompatibility("dumper_watcher");

TString TRTDumperState::GetType() const {
    return TRTDumperWatcher::GetTypeName();
}

void TRTDumperWatcher::AdvanceCursor(const TRecordsSetWT& records, ui64& lastEventId, const TString& eventIDColumn) const noexcept {
    for (auto&& record : records) {
        ui64 eventId = 0;
        if (!record.TryGet(eventIDColumn, eventId)) {
            continue;
        }
        lastEventId = Max<ui64>(eventId, lastEventId);
    }
}

bool TRTDumperWatcher::ProcessRecords(const TRecordsSetWT& records, const TExecutionContext& context, TProcessRecordsContext& processContext) const {
    if (!Dumper) {
        return false;
    }
    auto tableViewer = NCS::TDBTableFieldViewer(GetTableName(), GetDBName(), DBType, DBDefineSchema, context.GetServer());
    bool status = Dumper->ProcessRecords(records, tableViewer, processContext.IsDryRun());
    if (!status) {
        return false;
    }
    AdvanceCursor(records, processContext.MutableLastEventId(), GetEventIdColumn());
    return true;
}

NFrontend::TScheme TRTDumperWatcher::DoGetScheme(const IBaseServer& server) const {
    NFrontend::TScheme scheme = TBase::DoGetScheme(server);
    scheme.Add<TFSStructure>("dumper", "Дампер").SetStructure(NCS::TRTDumperContainer::GetScheme(server));
    scheme.Add<TFSVariants>("db_type", "Тип данных в таблице").SetVariants(TSet<TString>()).SetEditable(true);
    scheme.Add<TFSBoolean>("db_define_schema", "Тип данных в таблице задает YT схему");
    return scheme;
}

bool TRTDumperWatcher::DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
    TFLEventLog::Info("Deserializing dumper_watcher")("json_info", jsonInfo.GetStringRobust());
    if (!TBase::DoDeserializeFromJson(jsonInfo)) {
        TFLEventLog::Error("Failed to deserialize DumperWatcher, TBase::DoDeserializeFromJson returned false");
        return false;
    }
    JREAD_STRING_OPT(jsonInfo, "db_type", DBType);
    JREAD_BOOL_OPT(jsonInfo, "db_define_schema", DBDefineSchema);
    if (!jsonInfo.Has("dumper")) {
        TFLEventLog::Info("Deserializing dumper_watcher without dumper field");
        return Dumper.DeserializeFromJson(jsonInfo, "yt_dumper", "");
    }
    return Dumper.DeserializeFromJson(jsonInfo["dumper"]);
}

NJson::TJsonValue TRTDumperWatcher::DoSerializeToJson() const {
    NJson::TJsonValue result = TBase::DoSerializeToJson();
    JWRITE(result, "db_type", DBType);
    JWRITE(result, "db_define_schema", DBDefineSchema);
    result.InsertValue("dumper", Dumper.SerializeToJson());
    if (Dumper->GetClassName() == "yt_dumper") {
        static_cast<NCS::TYTDumper*>(Dumper.GetPtr().Get())->SerializeToJson(result);
    }
    TFLEventLog::Info("DumperWatcher serialization finished")("result", result.GetStringRobust());
    return result;
}
