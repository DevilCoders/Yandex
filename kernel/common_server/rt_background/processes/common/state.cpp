#include "state.h"


void TRTHistoryWatcherState::SerializeToProto(NCommonServerProto::THistoryProcessorData& proto) const {
    proto.SetLastEventId(LastEventId);
    if (LastError) {
        proto.SetLastError(LastError);
    }
}

bool TRTHistoryWatcherState::DeserializeFromProto(const NCommonServerProto::THistoryProcessorData& proto) {
    LastEventId = proto.GetLastEventId();
    LastError = proto.GetLastError();
    return true;
}

NJson::TJsonValue TRTHistoryWatcherState::GetReport() const {
    NJson::TJsonValue result = TBase::GetReport();
    JWRITE(result, "last_event_id", LastEventId);
    JWRITE(result, "last_error", LastError);
    return result;
}

NFrontend::TScheme TRTHistoryWatcherState::DoGetScheme() const {
    NFrontend::TScheme result = TBase::DoGetScheme();
    result.Add<TFSNumeric>("last_event_id");
    result.Add<TFSString>("last_error");
    return result;
}


void TRTInstantWatcherState::SerializeToProto(NCommonServerProto::TInstantProcessorData& proto) const {
    proto.SetLastExportTS(LastInstant.Seconds());
}

bool TRTInstantWatcherState::DeserializeFromProto(const NCommonServerProto::TInstantProcessorData& proto) {
    LastInstant = TInstant::Seconds(proto.GetLastExportTS());
    return true;
}

NJson::TJsonValue TRTInstantWatcherState::GetReport() const {
    NJson::TJsonValue result = TBase::GetReport();
    JWRITE_INSTANT(result, "last_instant", LastInstant);
    return result;
}

NFrontend::TScheme TRTInstantWatcherState::DoGetScheme() const {
    NFrontend::TScheme result = TBase::DoGetScheme();
    result.Add<TFSNumeric>("last_instant");
    return result;
}

