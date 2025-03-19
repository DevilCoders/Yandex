#include "time_of_day.h"
#include <kernel/common_server/util/json_processing.h>
#include <kernel/common_server/util/instant_model.h>
#include <util/string/printf.h>
#include <util/string/cast.h>
#include <util/generic/ymath.h>

bool TTimeOfDayInterval::Contains(const TTimeOfDay& fromExt, const TTimeOfDay& toExt, const TTimeOfDay& fromInt, const TTimeOfDay& toInt) {
    if (fromExt < toExt) {
        if (fromInt < toInt) {
            return (fromExt <= fromInt && toInt <= toExt);
        } else {
            return false;
        }
    } else {
        if (fromInt < toInt) {
            return (fromExt <= fromInt && fromExt <= toInt) || (fromInt < toExt&& toInt <= toExt);
        } else {
            return (fromExt <= fromInt && toInt <= toExt);
        }
    }
}

bool TTimeOfDayInterval::Contains(const TTimeOfDay& from, const TTimeOfDay& to, const TInstant instant) {
    return from.GetInstant(instant) <= instant && instant < to.GetInstant(instant);
}

TInstant TTimeOfDay::GetRight(const TInstant gmCurrent) const {
    const TInstant localCurrent = GetInstant(gmCurrent);
    if (localCurrent > gmCurrent) {
        return localCurrent;
    } else {
        return localCurrent + TDuration::Days(1);
    }
}

TString TTimeOfDay::SerializeToString(const TInstant value) {
    return TTimeOfDay(value).SerializeToString();
}

TString TTimeOfDay::SerializeToString(const bool withTimeZone) const {
    TStringStream ss;
    ss << Sprintf("%02d", Hours) + ":" + Sprintf("%02d", Minutes);
    if (MinutesShift && withTimeZone) {
        if (MinutesShift > 0) {
            ss << "+";
        } else {
            ss << "-";
        }
        ss << Sprintf("%02d", Abs(MinutesShift) / 60) + ":" + Sprintf("%02d", Abs(MinutesShift) % 60);
    }
    return ss.Str();
}

i64 TTimeOfDay::GetDurationSeconds() const {
    return (i64)Hours * 3600 + (i64)Minutes * 60 - (i64)MinutesShift * 60;
}

TInstant TTimeOfDay::GetInstant(const TInstant instant) const {
    return TInstant::Seconds(TInstant::Minutes(instant.Minutes() + MinutesShift).Days() * 3600 * 24 + GetDurationSeconds());
}

bool TTimeOfDay::DeserializeFromString(const TString& str) {
    TStringBuf sb(str.data(), str.size());
    return DeserializeFromString(sb);
}

bool TTimeOfDay::DeserializeFromString(const TStringBuf& sb) {
    auto gLogging = TFLRecords::StartContext().Method("TTimeOfDay::DeserializeFromString")("raw_data", sb);
    TStringBuf sTime;
    TStringBuf sShift;
    if (sb.TrySplit('+', sTime, sShift)) {
        TTimeOfDay tdShift;
        if (!tdShift.DeserializeFromString(sShift)) {
            TFLEventLog::Log("cannot parse timezone");
            return false;
        }
        MinutesShift = tdShift.GetHours() * 60 + tdShift.GetMinutes();
    } else if (sb.TrySplit('-', sTime, sShift)) {
        TTimeOfDay tdShift;
        if (!tdShift.DeserializeFromString(sShift)) {
            TFLEventLog::Log("cannot parse timezone");
            return false;
        }
        MinutesShift = -tdShift.GetHours() * 60 - tdShift.GetMinutes();
    } else {
        MinutesShift = 0;
        sTime = sb;
    }
    TStringBuf sHours;
    TStringBuf sMinutesSeconds;
    if (!sTime.TrySplit(':', sHours, sMinutesSeconds)) {
        TFLEventLog::Log("cannot find delimiter HH{:}MM[:SS]");
        return false;
    }
    if (!sHours) {
        Hours = 0;
    } else if (!TryFromString(sHours, Hours)) {
        TFLEventLog::Log("cannot parse hours value");
        return false;
    }

    TStringBuf sMinutes;
    TStringBuf sSeconds;

    if (!sMinutesSeconds.TrySplit(':', sMinutes, sSeconds)) {
        sMinutes = sMinutesSeconds;
    }

    if (!sMinutes) {
        Minutes = 0;
    } else if (!TryFromString(sMinutes, Minutes)) {
        TFLEventLog::Log("cannot parse minutes value");
        return false;
    }
    if (Hours > 24) {
        TFLEventLog::Log("incorrect hours value ( > 24)");
        return false;
    } else if (Hours == 24) {
        if (Minutes != 0) {
            TFLEventLog::Log("incorrect minutes value for 24 hours (00 only)");
            return false;
        }
        return true;
    } else {
        if (Minutes >= 60) {
            TFLEventLog::Log("incorrect minutes value: greater then 60");
            return false;
        }
        return true;
    }
}

bool TTimeOfDayInterval::DeserializeFromString(const TStringBuf& info, const char separator) {
    auto gLogging = TFLRecords::StartContext().Method("TTimeOfDayInterval::DeserializeFromString")("raw_data", info)("separator", separator);
    TStringBuf l;
    TStringBuf r;
    if (!info.TrySplit(separator, l, r)) {
        TFLEventLog::Log("cannot find separator");
        return false;
    }
    if (!Start.DeserializeFromString(l)) {
        TFLEventLog::Log("cannot parse start time of day");
        return false;
    }
    if (!Finish.DeserializeFromString(r)) {
        TFLEventLog::Log("cannot parse finish time of day");
        return false;
    }
    return true;
}

TString TTimeInterval::SerializeWithTimeZone(const TInstant val, const i32 timezoneMinutes) {
    if (!timezoneMinutes) {
        return val.ToString();
    } else {
        return val.FormatGmTime("%FT%X") + ((timezoneMinutes < 0) ? "-" : "+") + ::ToString(Abs(timezoneMinutes) / 60) + ":" + ::ToString(Abs(timezoneMinutes) % 60);
    }
}

TTimeInterval TTimeInterval::FromNow(const TDuration d) {
    const TInstant start = ModelingNow();
    return TTimeInterval(start, start + d);
}

NFrontend::TScheme TTimeInterval::GetScheme() {
    NFrontend::TScheme result;
    result.Add<TFSNumeric>("from", "From(UTC)").SetRequired(true);
    result.Add<TFSNumeric>("to", "To(UTC)").SetRequired(true);
    result.Add<TFSString>("hr_interval", "Начало интервала (YYYY-MM-DDTHH:mm:ss+HH:mm) / Конец интервала (YYYY-MM-DDTHH:mm:ss+HH:mm)").SetRequired(false);
    return result;
}

bool TTimeInterval::DeserializeFromString(const TStringBuf& sb, const char separator /*= '/'*/) {
    auto gLogging = TFLRecords::StartContext().Method("TTimeInterval::DeserializeFromString")("raw_data", sb)("separator", separator);
    TStringBuf sbFrom;
    TStringBuf sbTo;
    if (!sb.TrySplit(separator, sbFrom, sbTo)) {
        TFLEventLog::Log("Cannot split interval");
        return false;
    }
    if (!TInstant::TryParseIso8601(sbFrom, From)) {
        TFLEventLog::Log("Cannot parse from");
        return false;
    }
    if (!TInstant::TryParseIso8601(sbTo, To)) {
        TFLEventLog::Log("Cannot parse to");
        return false;
    }
    if (From > To) {
        TFLEventLog::Log("Incorrect interval (from > to)");
        return false;
    }
    return true;
}

bool TTimeInterval::IsInFuture() const {
    return To >= TInstantModel::Now();
}

void TTimeInterval::SerializeToProto(NCSProto::TTimeInterval& proto) const {
    proto.SetHrInterval(SerializeToString());
    proto.SetFrom(From.Seconds());
    proto.SetTo(To.Seconds());
}

bool TTimeInterval::DeserializeFromProto(const NCSProto::TTimeInterval& proto) {
    if (proto.HasHrInterval() && !!proto.GetHrInterval()) {
        if (!DeserializeFromString(proto.GetHrInterval())) {
            return false;
        }
    } else {
        From = TInstant::Seconds(proto.GetFrom());
        To = TInstant::Seconds(proto.GetTo());
    }
    if (!IsCorrectInterval()) {
        TFLEventLog::Log("incorrect interval (from > to)");
        return false;
    }
    return true;
}

bool TTimeInterval::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
    if (jsonInfo.IsString()) {
        return DeserializeFromString(jsonInfo.GetString());
    }
    if (jsonInfo.Has("from")) {
        if (!TJsonProcessor::Read(jsonInfo, "from", From, true)) {
            return false;
        }
        if (!TJsonProcessor::Read(jsonInfo, "to", To, true)) {
            return false;
        }
    } else if (jsonInfo.Has("min")) {
        if (!TJsonProcessor::Read(jsonInfo, "min", From, true)) {
            return false;
        }
        if (!TJsonProcessor::Read(jsonInfo, "max", To, true)) {
            return false;
        }
    } else if (jsonInfo.Has("hr_interval") && jsonInfo["hr_interval"].IsString()) {
        return DeserializeFromString(jsonInfo["hr_interval"].GetString());
    } else {
        TFLEventLog::Log("incorrect TTimeInterval json");
        return false;
    }
    if (!IsCorrectInterval()) {
        TFLEventLog::Log("check_interval_failed")("class_name", "TPlannedIntervalTime")("data", jsonInfo.GetStringRobust());
        return false;
    }
    return true;
}

NJson::TJsonValue TTimeInterval::SerializeToJson(const i32 timezoneMinutes /*= 0*/) const {
    NJson::TJsonValue result = NJson::JSON_MAP;
    result.InsertValue("from", SerializeWithTimeZone(From, timezoneMinutes));
    result.InsertValue("to", SerializeWithTimeZone(To, timezoneMinutes));
    result.InsertValue("hr_interval", SerializeToString('/', timezoneMinutes));
    return result;
}

TTimeOfDayInterval TTimeInterval::GetTimeOfDayInterval(const i32 timezoneMinutes) const {
    return TTimeOfDayInterval(TTimeOfDay(From, timezoneMinutes), TTimeOfDay(To, timezoneMinutes));
}

IOutputStream& operator<<(IOutputStream& out, const TTimeOfDay& x) {
    out << x.SerializeToString(true);
    return out;
}

template <>
bool TryFromStringImpl<TTimeOfDay>(const char* data, size_t len, TTimeOfDay& result) {
    return result.DeserializeFromString(TStringBuf(data, len));
}
