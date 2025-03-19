#include "state.h"

#include <kernel/common_server/util/json_processing.h>

#include <library/cpp/json/json_reader.h>

IRTBackgroundProcessState::TFactory::TRegistrator<IRTBackgroundProcessState> IRTBackgroundProcessState::Registrator(IRTBackgroundProcessState::GetTypeName());

NJson::TJsonValue TJsonRTBackgroundProcessState::GetReport() const {
    return SerializeToJson();
}

TBlob TJsonRTBackgroundProcessState::SerializeToBlob() const {
    return TBlob::FromString(SerializeToJson().GetStringRobust());
}

bool TJsonRTBackgroundProcessState::DeserializeFromBlob(const TBlob& data) {
    TStringBuf s(data.AsCharPtr(), data.Size());
    NJson::TJsonValue value;
    if (!NJson::ReadJsonFastTree(s, &value)) {
        ERROR_LOG << GetType() << ": cannot read Json from " << s << Endl;
        return false;
    }
    if (!DeserializeFromJson(value)) {
        ERROR_LOG << GetType() << ": cannot DeserializeFromJson " << value.GetStringRobust() << Endl;
        return false;
    }
    return true;
}

NJson::TJsonValue TRTBackgroundProcessStateContainer::GetReport() const {
    CHECK_WITH_LOG(!!ProcessState);
    NJson::TJsonValue result = ProcessState->GetReport();
    JWRITE(result, "bp_name", ProcessName);
    JWRITE(result, "type", ProcessState->GetType());
    JWRITE_INSTANT(result, "last_execution", LastExecution);
    JWRITE_INSTANT(result, "last_flush", LastFlush);
    JWRITE(result, "status", Status);
    JWRITE(result, "host", HostName);
    return result;
}

bool TRTBackgroundProcessStateContainer::DeserializeFromTableRecord(const NCS::NStorage::TTableRecordWT& record) {
    auto gLogging = TFLRecords::StartContext().Method("TRTBackgroundProcessStateContainer::DeserializeFromTableRecord")("raw_data", record.SerializeToString());
    ProcessName = record.GetString("bp_name");
    if (!ProcessName) {
        TFLEventLog::Error("incorrect bp_name");
        return false;
    }
    HostName = record.GetString("host");
    if (!record.TryGet("bp_last_execution", LastExecution)) {
        TFLEventLog::Error("incorrect bp_last_execution");
        return false;
    }

    TMaybe<NJson::TJsonValue> statusJson = record.CastTo<NJson::TJsonValue>("bp_status");
    if (!statusJson) {
        TFLEventLog::Error("incorrect bp_status");
        return false;
    }
    if (statusJson->IsString()) {
        Status = statusJson->GetString();
    } else {
        JREAD_INSTANT_OPT(*statusJson, "last_flush", LastFlush);
        JREAD_STRING_OPT(*statusJson, "status", Status);
        JREAD_STRING_OPT(*statusJson, "host", HostName);
    }

    ProcessState = IRTBackgroundProcessState::TFactory::Construct(record.GetString("bp_type"), "default");
    if (!ProcessState) {
        TFLEventLog::Error("cannot build class instance");
        return false;
    }
    auto dataBlob = record.GetBlobDeprecated("bp_state");
    if (!dataBlob) {
        TFLEventLog::Error("cannot parse bp_state as blob data");
        return false;
    }
    return ProcessState->DeserializeFromBlob(*dataBlob);
}

NStorage::TTableRecord TRTBackgroundProcessStateContainer::SerializeToTableRecord() const {
    CHECK_WITH_LOG(!!ProcessState);
    NStorage::TTableRecord result;
    result.Set("bp_name", ProcessName);
    result.Set("bp_type", ProcessState->GetType());
    result.Set("bp_last_execution", LastExecution);
    {
        NJson::TJsonValue statusData;
        statusData.InsertValue("status", Status);
        statusData.InsertValue("host", HostName);
        statusData.InsertValue("last_flush", LastFlush.Seconds());
        result.Set("bp_status", statusData.GetStringRobust());
    }
    auto blob = ProcessState->SerializeToBlob();
    result.SetBytesDeprecated("bp_state", blob.AsStringBuf());
    return result;
}
