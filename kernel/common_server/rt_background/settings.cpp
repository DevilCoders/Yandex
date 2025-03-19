#include "settings.h"
#include "state.h"

#include <kernel/common_server/common/scheme.h>
#include <kernel/common_server/library/logging/events.h>

#include <kernel/daemon/common/time_guard.h>

#include <util/generic/guid.h>
#include <kernel/common_server/library/tvm_services/abstract/abstract.h>

TString TRTBackgroundProcessContainer::GetClassName() const {
    if (!ProcessSettings) {
        return "undefined";
    } else {
        return ProcessSettings->GetType();
    }
}

NFrontend::TScheme TRTBackgroundProcessContainer::GetScheme(const IBaseServer& server) {
    NFrontend::TScheme result;
    result.Add<TFSNumeric>("bp_revision", "Версия конфигурации").SetReadOnly(true);
    result.Add<TFSString>("bp_name", "Идентификатор процесса").SetRequired(true);
    result.Add<TFSString>("_title").SetRequired(false).SetReadOnly(true);
    result.Add<TFSBoolean>("bp_enabled", "Enabled").SetDefault(false);
    result.Add<TFSWideVariants>("bp_type").InitVariants<IRTBackgroundProcess>(server).SetCustomStructureId("bp_settings");
    result.Add<TFSString>("background_process_state").SetVisual(TFSString::EVisualType::Json).MultiLine().ReadOnly().Required(false);
    return result;
}

bool TRTBackgroundProcessContainer::DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
    READ_DECODER_VALUE(decoder, values, Revision);
    READ_DECODER_VALUE(decoder, values, Name);
    READ_DECODER_VALUE(decoder, values, Enabled);
    auto gLogging = TFLRecords::StartContext()("values", JoinSeq(",", values));
    if (!Name) {
        TFLEventLog::Error("empty processor settings name");
        return false;
    }
    gLogging("name", Name);
    TString typeName;
    READ_DECODER_VALUE_TEMP(decoder, values, typeName, Type);

    ProcessSettings = IRTBackgroundProcess::TFactory::Construct(typeName);
    if (!ProcessSettings) {
        TFLEventLog::Error("incorrect rt_background object class")("class_name", typeName);
        return false;
    }

    ProcessSettings->SetRTProcessName(Name);

    NJson::TJsonValue jsonMeta;
    READ_DECODER_VALUE_JSON(decoder, values, jsonMeta, Meta);
    if (!ProcessSettings->DeserializeFromJson(jsonMeta)) {
        TFLEventLog::Error("cannot deserialize processor settings");
        return false;
    }

    return true;
}

TRTBackgroundProcessContainer TRTBackgroundProcessContainer::GetDeepCopyUnsafe() const {
    TRTBackgroundProcessContainer result;
    if (TBaseDecoder::DeserializeFromJson(result, SerializeToTableRecord().BuildWT().SerializeToJson())) {
        return result;
    }
    ALERT_LOG << "Failed deep copy for " << GetName() << Endl;
    Y_ASSERT(false);
    return TRTBackgroundProcessContainer();
}

TString TRTBackgroundProcessContainer::GetTitle() const {
    return (Enabled ? "E| " : "D| ")
        + TJsonProcessor::AlignString(GetClassName(), 30) + "| "
        + TJsonProcessor::AlignString(GetName(), 30) + "| "
        + ProcessSettings->GetHostFilter().GetTitle();
}

NJson::TJsonValue TRTBackgroundProcessContainer::GetReport() const {
    CHECK_WITH_LOG(!!ProcessSettings);
    NJson::TJsonValue result(NJson::JSON_MAP);
    if (HasRevision()) {
        JWRITE(result, "bp_revision", GetRevision());
    }
    JWRITE(result, "bp_settings", ProcessSettings->GetReport());
    JWRITE(result, "bp_type", ProcessSettings->GetType());
    JWRITE(result, "bp_name", Name);
    JWRITE(result, "_title", GetTitle());
    JWRITE(result, "bp_enabled", Enabled);
    return result;
}

NStorage::TTableRecord TRTBackgroundProcessContainer::SerializeToTableRecord() const {
    CHECK_WITH_LOG(!!ProcessSettings);
    NStorage::TTableRecord result;
    result.Set("bp_name", Name);
    result.Set("bp_enabled", Enabled);
    result.Set("bp_type", ProcessSettings->GetType());
    result.Set("bp_settings", ProcessSettings->SerializeToJson());
    if (HasRevision()) {
        result.Set("bp_revision", GetRevision());
    }
    return result;
}

bool TRTBackgroundProcessContainer::CheckOwner(const TString& userId) const {
    if (ProcessSettings->GetOwners().empty() || ProcessSettings->GetOwners().contains("*")) {
        return true;
    }
    return ProcessSettings->GetOwners().contains(userId);
}
bool TRTBackgroundProcessContainer::CheckOwner(const TSet<TString>& availableUserIds) const {
    if (availableUserIds.contains("*")) {
        return true;
    }
    if (ProcessSettings->GetOwners().empty() || ProcessSettings->GetOwners().contains("*")) {
        return true;
    }
    for (auto&& i : availableUserIds) {
        if (ProcessSettings->GetOwners().contains(i)) {
            return true;
        }
    }
    return false;
}

NFrontend::TScheme IRTRegularBackgroundProcess::DoGetScheme(const IBaseServer& server) const {
    NFrontend::TScheme result = TBase::DoGetScheme(server);
    auto gTab = result.StartTabGuard("execution");
    result.Add<TFSString>("timetable", "Расписание hhmm,hhmm,hhmm... запуска процесса", 90001).SetRequired(false);
    result.Add<TFSDuration>("period", "Период запуска процесса (если не указано расписание)", 90000).SetDefault(TDuration::Minutes(5));
    result.Add<TFSString>("robot_user_id", "Пользователь", 90000).SetDefault("ec65f4f0-8fa8-4887-bfdc-ca01c9906696").SetRequired(false);
    return result;
}

bool IRTRegularBackgroundProcess::DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
    JREAD_DURATION_OPT(jsonInfo, "period", Period);
    JREAD_STRING_OPT(jsonInfo, "robot_user_id", RobotUserId);
    if (GetUuid(RobotUserId).IsEmpty()) {
        return false;
    }
    TString timetableStr;
    JREAD_STRING_OPT(jsonInfo, "timetable", timetableStr);
    TSet<TString> timetableStrings;
    StringSplitter(timetableStr).SplitBySet(", ").SkipEmpty().Collect(&timetableStrings);
    for (auto&& i : timetableStrings) {
        ui16 tt;
        if (!TryFromString(i, tt)) {
            return false;
        }
        Timetable.emplace(tt);
    }
    if (Timetable.empty() && Period == TDuration::Zero()) {
        return false;
    }
    return TBase::DoDeserializeFromJson(jsonInfo);
}

TInstant IRTRegularBackgroundProcess::GetNextStartInstant(const TInstant lastCallInstant) const {
    if (Timetable.size()) {
        ui16 time = lastCallInstant.Minutes() % 60;
        time += (lastCallInstant.Hours() % 24) * 100;
        DEBUG_LOG << time << Endl;
        TMaybe<TInstant> result;
        for (auto&& i : Timetable) {
            if (i > time) {
                result = TInstant::Days(lastCallInstant.Days()) + TDuration::Hours(i / 100) + TDuration::Minutes(i % 100);
                break;
            }
        }
        if (!result) {
            result = TInstant::Days(lastCallInstant.Days() + 1) + TDuration::Hours(*Timetable.begin() / 100) + TDuration::Minutes(*Timetable.begin() % 100);
        }
        DEBUG_LOG << result->Seconds() << Endl;
        return Max(Now(), *result);
    } else {
        return lastCallInstant + GetPeriod();
    }
}

bool IRTBackgroundProcess::DeserializeFromJson(const NJson::TJsonValue& jsonSettings) {
    JREAD_STRING_OPT(jsonSettings, "bp_description", Description);
    JREAD_DURATION_OPT(jsonSettings, "freshness", Freshness);
    JREAD_DURATION_OPT(jsonSettings, "timeout", Timeout);
    if (jsonSettings.Has("host_filter") && !HostFilter.DeserializeFromJson(jsonSettings["host_filter"])) {
        return false;
    }
    if (jsonSettings.Has("bp_owners")) {
        if (!jsonSettings["bp_owners"].IsArray()) {
            return false;
        }
        for (auto&& i : jsonSettings["bp_owners"].GetArraySafe()) {
            TGUID tester;
            if (!GetUuid(i.GetString(), tester)) {
                return false;
            }
            Owners.emplace(i.GetString());
        }
    }
     if (jsonSettings.Has("time_restrictions")) {
        if (!TimeRestrictions.DeserializeFromJson(jsonSettings["time_restrictions"])) {
            return false;
        }
    }

    NJson::TJsonValue settings(jsonSettings);
    return TAttributedEntity::DeserializeAttributes(settings) && DoDeserializeFromJson(settings);
}

NJson::TJsonValue IRTBackgroundProcess::SerializeToJson() const {
    NJson::TJsonValue result = DoSerializeToJson();
    JWRITE(result, "bp_description", Description);
    TJsonProcessor::WriteContainerArray(result, "bp_owners", Owners, false);
    TJsonProcessor::WriteDurationString(result, "freshness", Freshness);
    if (Timeout) {
        TJsonProcessor::WriteDurationString(result, "timeout", *Timeout);
    }
    if (!TimeRestrictions.Empty()) {
        JWRITE(result, "time_restrictions", TimeRestrictions.SerializeToJson());
    }
    JWRITE(result, "host_filter", HostFilter.SerializeToJson());
    TAttributedEntity::SerializeAttributes(result);
    return result;
}

NFrontend::TScheme IRTBackgroundProcess::GetScheme(const IBaseServer& server) const {
    NFrontend::TScheme result = DoGetScheme(server);
    auto gTab = result.StartTabGuard("execution");
    result.Add<TFSString>("bp_description", "Описание процесса", 100000).SetMultiLine(true).SetRequired(true);
    result.Add<TFSStructure>("time_restrictions", "Ограничения времени запуска").SetStructure(TTimeRestrictionsPool::GetScheme()).SetRequired(false);
    result.Add<TFSStructure>("host_filter", "Фильтр хостов для исполнения процесса", 100001).SetStructure(HostFilter.GetScheme(server, server.GetCType())).SetRequired(true);
    result.Add<TFSDuration>("freshness", "Максимально допустимый возраст данных", 100002).SetDefault(TDuration::Minutes(1));
    {
        auto& arr = result.Add<TFSArray>("bp_owners", "Владельцы");
        arr.SetElement<TFSString>();
        arr.SetRequired(false);
    }
    TAttributedEntity::FillAttributesScheme(server, "rt_background", result);
    return result;
}

TAtomicSharedPtr<IRTBackgroundProcessState> IRTBackgroundProcess::Execute(TAtomicSharedPtr<IRTBackgroundProcessState> state, const TExecutionContext& context) const noexcept {
    const auto traceId = JoinStrings(SplitString(CreateGuidAsString(), "-"), "");
    auto lGuard = NExternalAPI::TSender::InitLinks(traceId);
    auto gLogging = TFLRecords::StartContext()("trace_id", traceId);

    auto evLogGuard = TFLRecords::StartContext().Source(GetRTProcessName());
    TTimeGuardImpl<false, TLOG_NOTICE> g("rt-background-" + GetType());
    StartInstant = Now();
    DataActuality = StartInstant - GetFreshness();
    try {
        return DoExecute(state, context);
    } catch (...) {
        ERROR_LOG << "Cannot execute rt-background process " << GetRTProcessName() << ": " << CurrentExceptionMessage() << Endl;
        return nullptr;
    }
}

TAtomicSharedPtr<IRTBackgroundProcessState> IRTRegularBackgroundProcess::Execute(TAtomicSharedPtr<IRTBackgroundProcessState> state, const TExecutionContext& context) const noexcept {
    auto evLogGuard = TFLRecords::StartContext()("user_id", GetRTProcessName());
    auto execResult = TBase::Execute(state, context);
    {
        TRTBackgroundProcessFinishedMessage mess(GetRTProcessName(), GetType());
        SendGlobalMessage(mess);
    }
    return execResult;
}
