#include "time_restriction.h"

#include <library/cpp/logger/global/global.h>
#include <util/string/split.h>
#include <util/string/strip.h>

const TVector<TTimeRestriction::EWeekDay> TTimeRestriction::WeekDays = { wdMonday, wdTuesday, wdWednesday, wdThursday, wdFriday, wdSaturday, wdSunday };
TVector<ui16> TTimeRestriction::DaysDecoder = {64, 1, 2, 4, 8, 16, 32};

TTimeRestriction::EWeekDay TTimeRestriction::GetDayOfWeekFromMonday(const i32 idx) {
    return WeekDays[idx % 7];
}

TTimeRestriction::EWeekDay TTimeRestriction::GetDayOfWeekFromSunday(const i32 idx) {
    return WeekDays[(idx + 6) % 7];
}

TTimeRestriction::TProto TTimeRestriction::SerializeToProto() const {
    TProto result;
    if (DateFrom != Max<ui16>()) {
        result.SetDateFrom(DateFrom);
    }
    if (DateTo != Max<ui16>()) {
        result.SetDateTo(DateTo);
    }
    if (TimeFrom != Max<ui16>()) {
        result.SetTimeFrom(TimeFrom);
    }
    if (TimeTo != Max<ui16>()) {
        result.SetTimeTo(TimeTo);
    }
    if (Day != Max<ui16>()) {
        result.SetDay(Day);
    }
    if (TimezoneShift != 0) {
        result.SetTimezoneShift(TimezoneShift);
    }

    return result;
}

TInstant TimeGmEx(const struct tm& time) {
    if (time.tm_year < 70) {
        return TInstant::Zero();
    }
    time_t seconds = 0;
    if (time.tm_year > 199) {
        ui64 yearExtra = time.tm_year - 199;
        auto copy = time;
        copy.tm_year = 199;
        seconds += TimeGM(&copy);
        seconds += yearExtra * 365 * 24 * 60 * 60;
    } else {
        seconds = TimeGM(&time);
    }
    return TInstant::Seconds(seconds);
}

void TTimeRestriction::BuildDatesInterval(const TInstant time, TInstant& dateFrom, TInstant& dateTo) const {
    if (DateFrom == Max<ui16>() || DateTo == Max<ui16>()) {
        dateFrom = TInstant::Zero();
        dateTo = TInstant::Max();
    } else {
        tm timeInfo;
        TInstant::Days(time.Days()).GmTime(&timeInfo);
        do {
            timeInfo.tm_mon = DateFrom / 100 - 1;
            timeInfo.tm_mday = DateFrom % 100;
            dateFrom = TimeGmEx(timeInfo);

            timeInfo.tm_mon = DateTo / 100 - 1;
            timeInfo.tm_mday = DateTo % 100;
            dateTo = TimeGmEx(timeInfo);

            if (dateTo <= dateFrom) {
                if (dateTo <= time) {
                    ++timeInfo.tm_year;
                    dateTo = TimeGmEx(timeInfo);
                } else {
                    timeInfo.tm_mon = DateFrom / 100 - 1;
                    timeInfo.tm_mday = DateFrom % 100;
                    --timeInfo.tm_year;
                    dateFrom = TimeGmEx(timeInfo);
                    ++timeInfo.tm_year;
                }
            }
            ++timeInfo.tm_year;
        } while (dateTo <= time);
    }
}

Y_FORCE_INLINE void TTimeRestriction::InsertSwitch(const TDuration value) {
    if ((ui64)Period == value.Seconds()) {
        CHECK_WITH_LOG(!DeltaToNextState.empty());
        if (DeltaToNextState.front() == 0) {
            DeltaToNextState.erase(DeltaToNextState.begin(), DeltaToNextState.begin() + 1);
            return;
        }
    }
    if (DeltaToNextState.empty() || (ui64)DeltaToNextState.back() != value.Seconds()) {
        DeltaToNextState.push_back(value.Seconds());
    } else {
        DeltaToNextState.pop_back();
    }
}

TString TTimeRestriction::SerializeDay(const ui32 day) const {
    TString result;
    ui32 dayMask = 1;
    for (ui32 i = 0; i < 7; ++i) {
        if (day & dayMask) {
            if (!!result)
                result += "|";
            result += ::ToString((EWeekDay)dayMask);
        }
        dayMask <<= 1;
    }
    return result;
}

bool TTimeRestriction::DeserializeDay(TString& info, ui16& result) const {
    result = 0;
    const TVector<TString> days = StringSplitter(info).SplitBySet("|").SkipEmpty().ToList<TString>();
    for (auto&& i : days) {
        EWeekDay day;
        if (!TryFromString<EWeekDay>(Strip(i), day)) {
            return false;
        }
        result |= (ui16)day;
    }
    return true;
}

bool TTimeRestriction::DeserializePair(ui16& date, ui16& time, TString& info) const {
    if (!info) {
        date = Max<ui16>();
        time = Max<ui16>();
    } else {
        size_t pos = info.find(':');
        if (pos == TString::npos) {
            return false;
        }
        TString strDate = Strip(info.substr(0, pos));
        TString strTime = Strip(info.substr(pos + 1));
        if (!strDate) {
            date = Max<ui16>();
        } else {
            if (!TryFromString(strDate, date)) {
                return false;
            }
        }
        if (!strTime) {
            time = Max<ui16>();
        } else {
            if (!TryFromString(strTime, time)) {
                return false;
            }
        }
    }
    return true;
}


void TTimeRestriction::SerializePair(const ui32 date, const ui32 time, IOutputStream& ss) const {
    if (date != Max<ui16>()) {
        ss << date;
    }
    if (date != Max<ui16>() || time != Max<ui16>()) {
        ss << ":";
    }
    if (time != Max<ui16>()) {
        ss << time;
    }
}

bool TTimeRestriction::DeserializeFromProto(const TProto& info) {
    if (info.HasDateFrom()) {
        DateFrom = info.GetDateFrom();
    }
    if (info.HasDateTo()) {
        DateTo = info.GetDateTo();
    }
    if (info.HasTimeFrom()) {
        TimeFrom = info.GetTimeFrom();
    }
    if (info.HasTimeTo()) {
        TimeTo = info.GetTimeTo();
    }
    if (info.HasDay()) {
        Day = info.GetDay();
    }
    if (info.HasTimezoneShift()) {
        TimezoneShift = info.GetTimezoneShift();
    }
    return true;
}

bool TTimeRestriction::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
    NJson::TJsonValue::TMapType mapInfo;
    if (!jsonInfo.GetMap(&mapInfo)) {
        TFLEventLog::Log("json_is_not_map")("method", "TTimeRestriction::DeserializeFromJson");
        return false;
    }

    if (!ReadField(mapInfo, "date_from", DateFrom)) {
        return false;
    }
    if (!ReadField(mapInfo, "date_to", DateTo)) {
        return false;
    }
    if (!ReadField(mapInfo, "time_from", TimeFrom)) {
        return false;
    }
    if (!ReadField(mapInfo, "time_to", TimeTo)) {
        return false;
    }
    if (!ReadField(mapInfo, "day", Day)) {
        return false;
    }
    if (!ReadField(mapInfo, "tz_shift", TimezoneShift)) {
        return false;
    }
    return true;
}

NFrontend::TScheme TTimeRestriction::GetScheme() {
    NFrontend::TScheme  scheme;
    scheme.Add<TFSNumeric>("time_from", "time_from").SetRequired(true);
    scheme.Add<TFSNumeric>("time_to", "time_to").SetRequired(true);
    scheme.Add<TFSNumeric>("day", "day mask: wdMonday = 1 ; wdSunday = 64 ").SetRequired(true);
    scheme.Add<TFSNumeric>("tz_shift", "tz_shift").SetDefault(3);
    scheme.Add<TFSNumeric>("date_from", "date_from");
    scheme.Add<TFSNumeric>("date_to", "date_to");
    return scheme;
}

bool TTimeRestriction::operator==(const TTimeRestriction& item) const {
    return
        DateFrom == item.DateFrom &&
        DateTo == item.DateTo &&
        TimeFrom == item.TimeFrom &&
        TimeTo == item.TimeTo &&
        Day == item.Day &&
        TimezoneShift == item.TimezoneShift;
}

NJson::TJsonValue TTimeRestriction::SerializeToJson(const bool needTimezone /*= true*/) const {
    NJson::TJsonValue result = NJson::JSON_NULL;
    if (DateFrom != Max<ui16>()) {
        result["date_from"] = DateFrom;
    }
    if (DateTo != Max<ui16>()) {
        result["date_to"] = DateTo;
    }
    if (TimeFrom != Max<ui16>()) {
        result["time_from"] = TimeFrom;
    }
    if (TimeTo != Max<ui16>()) {
        result["time_to"] = TimeTo;
    }
    if (needTimezone) {
        result["tz_shift"] = TimezoneShift;
    }
    if (Day != Max<ui16>()) {
        result["day"] = Day;
    }
    if (!result.IsDefined()) {
        result["time_from"] = 0;
        result["time_to"] = 2400;
    }
    return result;
}

bool TTimeRestriction::DeserializeFromString(const TString& info) {
    TVector<TString> fields = StringSplitter(info).SplitBySet("-").ToList<TString>();
    if (fields.size() != 3) {
        return false;
    }

    if (!DeserializePair(DateFrom, TimeFrom, fields[0])) {
        return false;
    }

    if (!DeserializePair(DateTo, TimeTo, fields[1])) {
        return false;
    }

    if (!fields[2]) {
        Day = Max<ui16>();
    } else {
        if (!TryFromString(StripInPlace(fields[2]), Day)) {
            if (!DeserializeDay(StripInPlace(fields[2]), Day)) {
                return false;
            }
        }
    }
    Compile();
    return true;
}

TString TTimeRestriction::SerializeAsString() const {
    TStringStream ss;
    SerializePair(DateFrom, TimeFrom, ss);
    ss << "-";
    SerializePair(DateTo, TimeTo, ss);
    ss << "-";
    if (Day != Max<ui16>()) {
        ss << SerializeDay(Day);
    }
    return ss.Str();
}

TDuration TTimeRestriction::GetCrossSize(const TInstant from, const TInstant to) const {
    TDuration result = TDuration::Zero();
    bool isInternal = IsActualNow(from);
    TInstant current = from;
    while (current < to) {
        const TInstant nextSwitch = Min(to, GetNextSwitching(current));
        if (isInternal) {
            result += nextSwitch - current;
        }
        current = nextSwitch;
        isInternal = !isInternal;
    }
    return result;
}

TInstant TTimeRestriction::GetNextSwitching(const TInstant time) const {
    TInstant timeLocal = Shift(time, 1);
    TInstant dateFrom;
    TInstant dateTo;
    BuildDatesInterval(timeLocal, dateFrom, dateTo);
    Y_ASSERT(dateFrom <= dateTo);

    if (dateFrom > timeLocal) {
        timeLocal = dateFrom;
        if (IsActualNow(timeLocal))
            return timeLocal;
    }

    if (Period == 0) {
        return dateTo;
    }

    i64 ds = (i64)timeLocal.Seconds() - BasePosition;
    i64 addressSeconds = ds % Period;
    i64 count = ds / Period;
    if (addressSeconds < 0) {
        addressSeconds += Period;
        count -= 1;
    }
    if (!DeltaToNextState.empty()) {
        for (ui32 i = 0; i < DeltaToNextState.size(); ++i) {
            if (DeltaToNextState[i] > addressSeconds) {
                TInstant result = TInstant::Seconds(BasePosition + count * Period + DeltaToNextState[i]);
                if (result < dateTo) {
                    return Shift(result, -1);
                } else {
                    return Shift(dateTo, -1);
                }
            }
        }
        TInstant result = TInstant::Seconds(BasePosition + (count + 1) * Period + DeltaToNextState[0]);
        if (result < dateTo) {
            return Shift(result, -1);
        } else {
            return Shift(dateTo, -1);
        }
    } else {
        return Shift(dateTo, -1);
    }
}

bool TTimeRestriction::IsSameDay(const TInstant dayInstant) const {
    tm timeInfo;
    Shift(dayInstant, +1).GmTime(&timeInfo);
    if (Day & DaysDecoder[timeInfo.tm_wday % 7]) {
        return true;
    }
    return false;
}

bool TTimeRestriction::IsActualNowTrivial(const TInstant now) const {
    if (DateFrom == Max<ui16>() && DateTo == Max<ui16>() && TimeFrom == Max<ui16>() && TimeTo == Max<ui16>() && Day == Max<ui16>()) {
        return true;
    }
    tm timeInfo;
    Shift(now, +1).GmTime(&timeInfo);
    DEBUG_LOG << Shift(now, +1).ToString() << " / " << timeInfo.tm_hour << ":" << timeInfo.tm_min << " / " << TimeFrom << " / " << TimeTo << Endl;
    if (DateFrom < DateTo) {
        if (DateFrom != Max<ui16>() && DateFrom > (timeInfo.tm_mon + 1) * 100 + timeInfo.tm_mday) {
            return false;
        }

        if (DateTo != Max<ui16>() && DateTo <= (timeInfo.tm_mon + 1) * 100 + timeInfo.tm_mday) {
            return false;
        }
    } else {
        if (DateFrom != Max<ui16>() && DateFrom > (timeInfo.tm_mon + 1) * 100 + timeInfo.tm_mday) {
            if (DateTo != Max<ui16>() && DateTo <= (timeInfo.tm_mon + 1) * 100 + timeInfo.tm_mday) {
                return false;
            }
        }

    }
    const ui32 time = timeInfo.tm_hour * 100 + timeInfo.tm_min;
    if (TimeFrom < TimeTo) {
        if (TimeFrom != Max<ui16>() && TimeFrom > time) {
            return false;
        }

        if (TimeTo != Max<ui16>() && TimeTo <= time) {
            return false;
        }
    } else {
        if (TimeFrom != Max<ui16>() && TimeFrom > time) {
            if (TimeTo != Max<ui16>() && TimeTo <= time) {
                return false;
            }
        }

    }

    if (Day != Max<ui16>()) {
        ui32 day = timeInfo.tm_wday;
        if (TimeFrom == Max<ui16>() || TimeTo == Max<ui16>() || (TimeFrom == TimeTo)) {
            if ((Day & DaysDecoder[day]) == 0)
                return false;
        } else {
            if (TimeFrom < TimeTo) {
                if ((Day & DaysDecoder[day]) == 0)
                    return false;
            } else {
                const ui32 predDay = day ? day - 1 : 6;
                const bool predDayActive = Day & DaysDecoder[predDay];
                const bool currDayActive = Day & DaysDecoder[day];
                if (currDayActive) {
                    if (!predDayActive && TimeTo > time)
                        return false;
                } else {
                    if (!predDayActive)
                        return false;
                    if (time > TimeTo)
                        return false;
                }
            }
        }
    }

    return true;
}

bool TTimeRestriction::IsActualNow(const TInstant time) const {
    TInstant timeLocal = Shift(time, 1);
    TInstant dateFrom;
    TInstant dateTo;
    BuildDatesInterval(timeLocal, dateFrom, dateTo);
    Y_ASSERT(dateFrom <= dateTo);

    if (dateFrom > timeLocal) {
        return false;
    }

    if (!Period) {
        return timeLocal < dateTo;
    }

    i64 ds = (i64)timeLocal.Seconds() - BasePosition;
    i64 addressSeconds = ds % Period;
    if (addressSeconds < 0) {
        addressSeconds += Period;
    }

    for (ui32 i = 0; i < DeltaToNextState.size(); ++i) {
        if (DeltaToNextState[i] > addressSeconds) {
            return (((StartIsActive ? 1 : 0) + i) % 2) == 1;
        }
    }
    return StartIsActive;
}

bool TTimeRestriction::Compile() {
    TDuration timeFrom = (TimeFrom == Max<ui16>() || TimeFrom == TimeTo) ? TDuration::Zero() : (TDuration::Hours(TimeFrom / 100) + TDuration::Minutes(TimeFrom % 100));
    TDuration timeTo = (TimeTo == Max<ui16>() || TimeFrom == TimeTo) ? TDuration::Days(1) : (TDuration::Hours(TimeTo / 100) + TDuration::Minutes(TimeTo % 100));
    DeltaToNextState.clear();
    if ((Day & Max<ui8>()) == Max<ui8>()) {
        if (TimeFrom == Max<ui16>() && TimeTo == Max<ui16>()) {
            Period = 0;
        } else {
            Period = TDuration::Days(1).Seconds();
            BasePosition = TInstant::Days(Now().Days()).Seconds() - Period * 10000;
            StartIsActive = (TimeFrom == TimeTo) ? true : (timeFrom > timeTo);
            if (TimeFrom != TimeTo) {
                if (timeTo > timeFrom) {
                    DeltaToNextState.push_back(timeFrom.Seconds());
                    DeltaToNextState.push_back(timeTo.Seconds());
                } else {
                    DeltaToNextState.push_back(timeTo.Seconds());
                    DeltaToNextState.push_back(timeFrom.Seconds());
                }
            }
        }
    } else {
        TInstant base = Now();
        tm timeInfo;
        base.GmTime(&timeInfo);
        Period = TDuration::Days(7).Seconds();
        BasePosition = TInstant::Days(base.Days() - timeInfo.tm_wday).Seconds() - Period * 100;
        ui32 predI = 6;
        StartIsActive = (DaysDecoder[predI] & Day) && (timeFrom > timeTo || timeTo == timeFrom + TDuration::Days(1));
        for (ui32 i = 0; i < 7; ++i) {
            if (DaysDecoder[i] & Day) {
                if (timeTo > timeFrom) {
                    InsertSwitch(TDuration::Days(i) + timeFrom);
                    InsertSwitch(TDuration::Days(i) + timeTo);
                } else {
                    if ((DaysDecoder[predI] & Day) != 0) {
                        InsertSwitch(TDuration::Days(i) + timeTo);
                    }
                    InsertSwitch(TDuration::Days(i) + timeFrom);
                    if (i < 6 && (DaysDecoder[i + 1] & Day) == 0) {
                        InsertSwitch(TDuration::Days(i + 1) + timeTo);
                    }
                }
            } else if (i == 0 && (DaysDecoder[predI] & Day) && timeFrom >= timeTo) {
                InsertSwitch(TDuration::Days(i) + timeTo);
            }
            predI = i;
        }
    }
    CHECK_WITH_LOG(DeltaToNextState.size() % 2 == 0);
    return true;
}

TInstant TTimeRestriction::GetTime(TInstant now, const ui16 hhmm) const {
    tm timeInfo;
    now.GmTime(&timeInfo);
    now -= TDuration::Hours(timeInfo.tm_hour);
    now -= TDuration::Minutes(timeInfo.tm_min);
    now -= TDuration::Seconds(timeInfo.tm_sec);
    if (hhmm != Max<ui16>()) {
        now += TDuration::Hours(hhmm / 100 - TimezoneShift) + TDuration::Minutes(hhmm % 100);
    }
    return now;
}

TInstant TTimeRestriction::Shift(TInstant timestamp, i16 coefficient) const {
    if (coefficient * TimezoneShift >= 0) {
        return timestamp + TDuration::Hours(std::abs(TimezoneShift * coefficient));
    } else {
        return timestamp - TDuration::Hours(std::abs(TimezoneShift * coefficient));
    }
}
