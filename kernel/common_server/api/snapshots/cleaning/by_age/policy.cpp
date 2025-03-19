#include "policy.h"
#include <kernel/common_server/api/snapshots/object.h>
#include <kernel/common_server/api/snapshots/controller.h>

namespace NCS {
    TByAgeCleaningPolicy::TFactory::TRegistrator<TByAgeCleaningPolicy> TByAgeCleaningPolicy::Registrator(TByAgeCleaningPolicy::GetTypeName());

    NJson::TJsonValue TByAgeCleaningPolicy::SerializeToJson() const {
        NJson::TJsonValue result = TBase::SerializeToJson();
        TJsonProcessor::Write(result, "allowed_age_seconds", AllowedAgeSeconds);
        TJsonProcessor::Write(result, "timestamp_field_name", TimestampFieldName);
        return result;
    }

    bool TByAgeCleaningPolicy::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
        if (!TBase::DeserializeFromJson(jsonInfo)) {
            return false;
        }
        if (!TJsonProcessor::Read(jsonInfo, "allowed_age_seconds", AllowedAgeSeconds)) {
            return false;
        }
        if (!TJsonProcessor::Read(jsonInfo, "timestamp_field_name", TimestampFieldName)) {
            return false;
        }
        return true;
    }

    NFrontend::TScheme TByAgeCleaningPolicy::GetScheme(const IBaseServer& server) const {
        NFrontend::TScheme result = TBase::GetScheme(server);
        result.Add<TFSNumeric>("allowed_age_seconds", "Максимальный возраст объектов в снепшоте (в секундах)").SetRequired(true);
        result.Add<TFSString>("timestamp_field_name", "Имя поля с timestamp").SetRequired(true);
        return result;
    }

    bool TByAgeCleaningPolicy::PrepareCleaningCondition(TSRCondition& cleaningCondition) const {
        const auto minAllowedTimestamp = ModelingNow() - TDuration::Seconds(AllowedAgeSeconds);
        auto& srMulti = cleaningCondition.Ret<TSRMulti>();
        srMulti.InitNode<TSRBinary>(TimestampFieldName, minAllowedTimestamp.Seconds(), NCS::NStorage::NRequest::EBinaryOperation::Less);
        return true;
    }

}
