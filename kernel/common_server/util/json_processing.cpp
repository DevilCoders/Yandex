#include "json_processing.h"

#include <kernel/common_server/util/datetime/datetime.h>


const NJson::TJsonValue& TJsonProcessor::GetAvailableElement(const NJson::TJsonValue& jsonBase, const TVector<TString>& children) {
    for (auto&& i : children) {
        if (jsonBase.Has(i)) {
            return jsonBase[i];
        }
    }
    return Default<NJson::TJsonValue>();
}

TString TJsonProcessor::FormatDurationString(const TDuration value) {
    return NUtil::FormatDuration(value);
}

void TJsonProcessor::WriteDurationInt(NJson::TJsonValue& target, const TString& fieldName, const TDuration value, const TMaybe<TDuration> defValue) {
    CHECK_WITH_LOG(target.IsMap() || !target.IsDefined());
    Y_ASSERT(!target.Has(fieldName));
    if (defValue && *defValue == value) {
        return;
    }
    target.InsertValue(fieldName, value.Seconds());
}

bool TJsonProcessor::Read(const NJson::TJsonValue& jsonInfo, const TString& fieldName, TDuration& result, const bool mustBe) {
    const NJson::TJsonValue* jsonValue;
    if (!jsonInfo.GetValuePointer(fieldName, &jsonValue)) {
        if (mustBe) {
            TFLEventLog::Error("no_field")("field", fieldName);
        }
        return !mustBe;
    }
    if (jsonValue->IsUInteger()) {
        result = TDuration::Seconds(jsonValue->GetUInteger());
        return true;
    }
    if (jsonValue->IsString()) {
        if (TDuration::TryParse(jsonValue->GetString(), result)) {
            return true;
        }
        TFLEventLog::Error("cannot_parse_duration")("field", fieldName)("value", jsonValue->GetString());
    }
    TFLEventLog::Error("incorrect_type")("field", fieldName)("value", jsonValue->GetString())("type", jsonValue->GetType());
    return false;
}

bool TJsonProcessor::Read(const NJson::TJsonValue& jsonInfo, const TString& fieldName, TInstant& result, const bool mustBe /*= false*/) {
    const NJson::TJsonValue* jsonValue;
    if (!jsonInfo.GetValuePointer(fieldName, &jsonValue)) {
        if (mustBe) {
            TFLEventLog::Error("no_field")("field", fieldName);
        }
        return !mustBe;
    }
    if (jsonValue->IsUInteger()) {
        result = TInstant::Seconds(jsonValue->GetUInteger());
        return true;
    }
    if (jsonValue->IsString()) {
        if (TInstant::TryParseIso8601(jsonValue->GetString(), result)) {
            return true;
        }
        TFLEventLog::Error("cannot_parse_instant")("field", fieldName)("value", jsonValue->GetString());
    }
    TFLEventLog::Error("incorrect_type")("field", fieldName)("value", jsonValue->GetString())("type", jsonValue->GetType());
    return false;
}

bool TJsonProcessor::Read(const NJson::TJsonValue& jsonInfo, const TString& fieldName, TString& result, const bool mustBe, const bool mayBeEmpty) {
    auto gLogging = TFLRecords::StartContext()("field", fieldName);
    const NJson::TJsonValue* jsonValue;
    if (!jsonInfo.GetValuePointer(fieldName, &jsonValue)) {
        if (mustBe) {
            TFLEventLog::Error("no_field");
        }
        return !mustBe;
    }
    gLogging("value", jsonValue->GetString())("type", jsonValue->GetType());
    if (!jsonValue->IsString()) {
        TFLEventLog::Error("incorrect_type");
        return false;
    }
    result = jsonValue->GetString();
    if (!mayBeEmpty && !result) {
        TFLEventLog::Error("empty_string");
        return false;
    }
    return true;
}

bool TJsonProcessor::Read(const NJson::TJsonValue& jsonInfo, const TString& fieldName, ui32& result, const bool mustBe, const bool canBeZero) {
    const NJson::TJsonValue* jsonValue;
    if (!jsonInfo.GetValuePointer(fieldName, &jsonValue)) {
        if (mustBe) {
            TFLEventLog::Error("no_field")("field", fieldName);
        }
        return !mustBe;
    }
    if (jsonValue->IsUInteger()) {
        result = jsonValue->GetUInteger();
        if (!canBeZero && !result) {
            TFLEventLog::Error("incorrect_field_value")("field", fieldName)("value", result);
            return false;
        }
        return true;
    }
    TFLEventLog::Error("incorrect_type")("field", fieldName)("value", jsonValue->GetString())("type", jsonValue->GetType());
    return false;
}

bool TJsonProcessor::Read(const NJson::TJsonValue& jsonInfo, const TString& fieldName, ui64& result, const bool mustBe, const bool canBeZero) {
    const NJson::TJsonValue* jsonValue;
    if (!jsonInfo.GetValuePointer(fieldName, &jsonValue)) {
        if (mustBe) {
            TFLEventLog::Error("no_field")("field", fieldName);
        }
        return !mustBe;
    }
    if (jsonValue->IsUInteger()) {
        result = jsonValue->GetUInteger();
        if (!canBeZero && !result) {
            TFLEventLog::Error("incorrect_field_value")("field", fieldName)("value", result);
            return false;
        }
        return true;
    }
    TFLEventLog::Error("incorrect_type")("field", fieldName)("value", jsonValue->GetString())("type", jsonValue->GetType());
    return false;
}

bool TJsonProcessor::Read(const NJson::TJsonValue& jsonInfo, const TString& fieldName, i32& result, const bool mustBe, const bool canBeZero) {
    const NJson::TJsonValue* jsonValue;
    if (!jsonInfo.GetValuePointer(fieldName, &jsonValue)) {
        if (mustBe) {
            TFLEventLog::Error("no_field")("field", fieldName);
        }
        return !mustBe;
    }
    if (jsonValue->IsInteger()) {
        result = jsonValue->GetInteger();
        if (!canBeZero && !result) {
            TFLEventLog::Error("incorrect_field_value")("field", fieldName)("value", result);
            return false;
        }
        return true;
    }
    TFLEventLog::Error("incorrect_type")("field", fieldName)("value", jsonValue->GetString())("type", jsonValue->GetType());
    return false;
}

bool TJsonProcessor::Read(const NJson::TJsonValue& jsonInfo, const TString& fieldName, i64& result, const bool mustBe, const bool canBeZero) {
    const NJson::TJsonValue* jsonValue;
    if (!jsonInfo.GetValuePointer(fieldName, &jsonValue)) {
        if (mustBe) {
            TFLEventLog::Error("no_field")("field", fieldName);
        }
        return !mustBe;
    }
    if (jsonValue->IsInteger()) {
        result = jsonValue->GetInteger();
        if (!canBeZero && !result) {
            TFLEventLog::Error("incorrect_field_value")("field", fieldName)("value", result);
            return false;
        }
        return true;
    }
    TFLEventLog::Error("incorrect_type")("field", fieldName)("value", jsonValue->GetString())("type", jsonValue->GetType());
    return false;
}

bool TJsonProcessor::Read(const NJson::TJsonValue& jsonInfo, const TString& fieldName, bool& result, const bool mustBe) {
    const NJson::TJsonValue* jsonValue;
    if (!jsonInfo.GetValuePointer(fieldName, &jsonValue)) {
        if (mustBe) {
            TFLEventLog::Error("no_field")("field", fieldName);
        }
        return !mustBe;
    }
    if (jsonValue->IsBoolean()) {
        result = jsonValue->GetBoolean();
        return true;
    }
    TFLEventLog::Error("incorrect_type")("field", fieldName)("value", jsonValue->GetString())("type", jsonValue->GetType());
    return false;
}

void TJsonProcessor::WriteInstant(NJson::TJsonValue& target, const TString& fieldName, const TInstant value, const TMaybe<TInstant> defValue) {
    CHECK_WITH_LOG(target.IsMap() || !target.IsDefined());
    Y_ASSERT(!target.Has(fieldName));
    if (defValue && *defValue == value) {
        return;
    }
    target.InsertValue(fieldName, value.Seconds());
}

void TJsonProcessor::WriteDurationString(NJson::TJsonValue& target, const TString& fieldName, const TDuration value, const TMaybe<TDuration> defValue) {
    CHECK_WITH_LOG(target.IsMap() || !target.IsDefined());
    Y_ASSERT(!target.Has(fieldName));
    if (defValue && *defValue == value) {
        return;
    }
    target.InsertValue(fieldName, FormatDurationString(value));
}
