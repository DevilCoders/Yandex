#pragma once
#include <kernel/common_server/library/time_restriction/proto/restrictions.pb.h>
#include <kernel/common_server/common/scheme.h>
#include <kernel/common_server/util/time_of_day.h>
#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/util/accessor.h>
#include <util/generic/object_counter.h>

class TTimeRestriction: public TObjectCounter<TTimeRestriction> {
public:
    enum EWeekDay {
        wdMonday = 1 /* "monday" */,
        wdTuesday = 2 /* "tuesday" */,
        wdWednesday = 4 /* "wednesday" */,
        wdThursday = 8 /* "thursday" */,
        wdFriday = 16 /* "friday" */,
        wdSaturday = 32 /* "saturday" */,
        wdSunday = 64 /* "sunday" */
    };

    static const ui16 WorkDays = wdMonday | wdTuesday | wdWednesday | wdThursday | wdFriday;
    static const ui16 Weekends = wdSaturday | wdSunday;
    static const TVector<EWeekDay> WeekDays;
    CSA_READONLY(ui16, DateFrom, Max<ui16>());
    CSA_READONLY(ui16, DateTo, Max<ui16>());
    CSA_READONLY(ui16, TimeFrom, Max<ui16>());
    CSA_READONLY(ui16, TimeTo, Max<ui16>());
    CSA_READONLY(ui16, Day, Max<ui16>());
    CSA_READONLY(i16, TimezoneShift, 0);
protected:

    i64 BasePosition;
    TVector<i64> DeltaToNextState;
    bool StartIsActive;
    i64 Period = 0;
    static TVector<ui16> DaysDecoder;

    void BuildDatesInterval(const TInstant time, TInstant& dateFrom, TInstant& dateTo) const;

    Y_FORCE_INLINE void InsertSwitch(const TDuration value);

    Y_WARN_UNUSED_RESULT bool DeserializeDay(TString& info, ui16& result) const;
    Y_WARN_UNUSED_RESULT bool DeserializePair(ui16& date, ui16& time, TString& info) const;

    TString SerializeDay(const ui32 day) const;
    void SerializePair(const ui32 date, const ui32 time, IOutputStream& ss) const;
private:
    using TProto = NRTLineProto::TTimeRestriction;
    template <class T>
    bool ReadField(const NJson::TJsonValue::TMapType& mapInfo, const TString& fieldName, T& result) const {
        auto it = mapInfo.find(fieldName);
        if (it == mapInfo.end()) {
            return true;
        }
        if (!it->second.IsInteger()) {
            TFLEventLog::Log("incorrect_field_type")("field_name", fieldName)("type", it->second.GetType());
            return false;
        }
        result = it->second.GetInteger();
        return true;
    }
public:
    static EWeekDay GetDayOfWeekFromMonday(const i32 idx);
    static EWeekDay GetDayOfWeekFromSunday(const i32 idx);

    TProto SerializeToProto() const;
    Y_WARN_UNUSED_RESULT bool DeserializeFromProto(const TProto& info);
    NJson::TJsonValue SerializeToJson(const bool needTimezone = true) const;
    Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);

    static NFrontend::TScheme GetScheme();
    bool operator==(const TTimeRestriction& item) const;

    Y_WARN_UNUSED_RESULT bool DeserializeFromString(const TString& info);
    TString SerializeAsString() const;

    TDuration GetCrossSize(const TInstant from, const TInstant to) const;
    TInstant GetNextSwitching(const TInstant time) const;
    bool IsSameDay(const TInstant dayInstant) const;
    bool IsActualNow(const TInstant time) const;
    bool Compile();

    TTimeOfDay GetTimeOfDayFrom(const ui16 defaultValue = Max<ui16>()) const {
        return TTimeOfDay((TimeFrom == Max<ui16>()) ? defaultValue : TimeFrom, TimezoneShift * 60);
    }

    TTimeOfDay GetTimeOfDayTo(const ui16 defaultValue = Max<ui16>()) const {
        return TTimeOfDay((TimeTo == Max<ui16>()) ? defaultValue : TimeTo, TimezoneShift * 60);
    }

    TTimeRestriction& SetTimeRestriction(const ui16 from = Max<ui16>(), const ui16 to = Max<ui16>()) {
        TimeFrom = from;
        TimeTo = to;
        Compile();
        return *this;
    }

    TTimeRestriction& SetDateRestriction(const ui16 from = Max<ui16>(), const ui16 to = Max<ui16>()) {
        DateFrom = from;
        DateTo = to;
        Compile();
        return *this;
    }

    TTimeRestriction& SetDayRestriction(const ui16 day = Max<ui16>()) {
        Day = day;
        Compile();
        return *this;
    }

    TTimeRestriction& SetTimezoneShift(i16 shift) {
        TimezoneShift = shift;
        Compile();
        return *this;
    }

    TInstant GetTimeStart(const TInstant reference) const {
        return Shift(GetTime(reference, TimeFrom), -1);
    }

    TInstant GetTimeFinish(const TInstant reference) const {
        return Shift(GetTime(reference, TimeTo), -1);
    }

    bool IsActualNowTrivial(const TInstant now) const;

private:
    TInstant GetTime(TInstant reference, const ui16 hhmm) const;
    TInstant Shift(TInstant timestamp, i16 coefficient) const;
};
