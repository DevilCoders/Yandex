#pragma once
#include <kernel/common_server/util/accessor.h>
#include <util/datetime/base.h>
#include <kernel/common_server/util/datetime/datetime.h>
#include <kernel/common_server/util/proto/common.pb.h>
#include <kernel/common_server/common/scheme.h>

class TTimeOfDayInterval;

class TTimeInterval {
private:
    CS_ACCESS(TTimeInterval, TInstant, From, TInstant::Zero());
    CS_ACCESS(TTimeInterval, TInstant, To, TInstant::Zero());
public:
    TTimeInterval() = default;
    TTimeInterval(const TInstant from, const TInstant to)
        : From(from)
        , To(to) {

    }

    explicit TTimeInterval(const TInstant from)
        : From(from)
        , To(from) {

    }

    static TString SerializeWithTimeZone(const TInstant val, const i32 timezoneMinutes);

    static TTimeInterval FromNow(const TDuration d);
    static NFrontend::TScheme GetScheme();

    bool operator<(const TTimeInterval& interval) const {
        return From < interval.From;
    }

    TTimeInterval operator+(const TDuration d) const {
        return TTimeInterval(From + d, To + d);
    }

    bool IsBefore(const TTimeInterval& interval) const {
        return GetTo() <= interval.GetFrom();
    }

    TString ToString(const char separator = '/', const i32 timezoneMinutes = 0) const {
        return SerializeWithTimeZone(From, timezoneMinutes) + separator + SerializeWithTimeZone(To, timezoneMinutes);
    }

    TString SerializeToString(const char separator = '/', const i32 timezoneMinutes = 0) const {
        return ToString(separator, timezoneMinutes);
    }
    Y_WARN_UNUSED_RESULT bool DeserializeFromString(const TString& strData, const char separator = '/') {
        TStringBuf sb(strData.data(), strData.size());
        return DeserializeFromString(sb, separator);
    }

    Y_WARN_UNUSED_RESULT bool DeserializeFromString(const TStringBuf& sb, const char separator = '/');


    TInstant GetStart() const {
        return From;
    }

    TInstant GetFinish() const {
        return To;
    }

    bool IsSameDay(const TTimeZoneHelper timeZone) const {
        return timeZone.Local(From).Days() == timeZone.Local(To).Days();
    }

    TTimeInterval& SetMin(const TInstant min) {
        From = min;
        return *this;
    }

    TTimeInterval& SetMax(const TInstant max) {
        To = max;
        return *this;
    }

    TInstant GetMin() const {
        return From;
    }

    TInstant GetMax() const {
        return To;
    }

    TInstant& MutableMin() {
        return From;
    }

    TInstant& MutableMax() {
        return To;
    }

    bool IsEqual(const TTimeInterval& item) const {
        if (From != item.From) {
            return false;
        }
        if (To != item.To) {
            return false;
        }
        return true;
    }

    TInstant GetCentralInstant() const {
        return From + 0.5 * (To - From);
    }

    bool IsInitialized() const {
        return From || To;
    }

    bool IsEmpty() const {
        return !From && !To;
    }

    bool Contains(const TTimeInterval& interval) const {
        return Contains(interval.GetFrom(), interval.GetTo());
    }

    bool Contains(const TInstant instant) const {
        return From <= instant && instant < To;
    }

    bool Contains(const TInstant from, const TInstant to) const {
        return From <= from && to <= To;
    }

    TDuration GetLength() const {
        return To - From;
    }

    TDuration GetIntervalDuration() const {
        return GetLength();
    }

    bool IsCorrectInterval() const {
        return To >= From;
    }

    bool IsInFuture() const;

    void DoUnite(const TTimeInterval& additional) {
        From = Min(From, additional.From);
        To = Max(To, additional.To);
    }

    TTimeInterval GetUnite(const TTimeInterval& additional) const {
        TTimeInterval result;
        result.From = Min(From, additional.From);
        result.To = Max(To, additional.To);
        return result;
    }

    TMaybe<TTimeInterval> Intersect(const TTimeInterval& other) const {
        const auto left = Max(From, other.From);
        const auto right = Max(To, other.To);
        if (right < left) {
            return Nothing();
        }
        return TTimeInterval(left, right);
    }

    bool Intersected(const TTimeInterval& other) const {
        const auto left = Max(From, other.From);
        const auto right = Min(To, other.To);
        return right > left;
    }

    void SerializeToProto(NCSProto::TTimeInterval& proto) const;
    Y_WARN_UNUSED_RESULT bool DeserializeFromProto(const NCSProto::TTimeInterval& proto);
    Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
    NJson::TJsonValue SerializeToJson(const i32 timezoneMinutes = 0) const;
    TTimeOfDayInterval GetTimeOfDayInterval(const i32 timezoneMinutes = 0) const;
};

class TTimeOfDay {
private:
    CS_ACCESS(TTimeOfDay, i32, MinutesShift, 0);
    CSA_READONLY(ui32, Hours, 0);
    CSA_READONLY(ui32, Minutes, 0);
public:
    ui16 GetValueNoTimezone() const {
        return Hours * 100 + Minutes;
    }

    TTimeOfDay() = default;
    TTimeOfDay(const ui16 value, const i32 minutesShift = 0)
        : MinutesShift(minutesShift)
    {
        Hours = value / 100;
        Minutes = value % 100;
    }

    TTimeOfDay(const ui16 value, const TTimeZoneHelper timeZone)
        : TTimeOfDay(value, timeZone.Minutes()) {
    }

    static TTimeOfDay Moscow(const ui16 timeValue) {
        return TTimeOfDay(timeValue, 180);
    }

    TTimeOfDay(const TInstant value, const i32 timezoneMinutes = 0) {
        Hours = value.Hours() % 24;
        Minutes = value.Minutes() % 60;
        MinutesShift = timezoneMinutes;
    }

    TInstant GetRight(const TInstant gmCurrent) const;

    TString SerializeToString(const bool withTimeZone = false) const;
    static TString SerializeToString(const TInstant value);

    bool operator<(const TTimeOfDay& item) const {
        return GetDurationSeconds() < item.GetDurationSeconds();
    }

    bool operator<=(const TTimeOfDay& item) const {
        return GetDurationSeconds() <= item.GetDurationSeconds();
    }

    i64 GetDurationSeconds() const;

    TInstant GetInstant(const TInstant instant) const;

    Y_WARN_UNUSED_RESULT bool DeserializeFromString(const TString& str);
    Y_WARN_UNUSED_RESULT bool DeserializeFromString(const TStringBuf& str);
};

IOutputStream& operator<<(IOutputStream& out, const TTimeOfDay& x);

template <>
bool TryFromStringImpl<TTimeOfDay>(const char* data, size_t len, TTimeOfDay& result);

class TTimeOfDayInterval {
private:
    CSA_DEFAULT(TTimeOfDayInterval, TTimeOfDay, Start);
    CSA_DEFAULT(TTimeOfDayInterval, TTimeOfDay, Finish);
public:
    TTimeOfDayInterval() = default;

    static bool Contains(const TTimeOfDay& fromExt, const TTimeOfDay& toExt, const TTimeOfDay& fromInt, const TTimeOfDay& toInt);
    static bool Contains(const TTimeOfDay& from, const TTimeOfDay& to, const TInstant instant);

    bool Contains(const TTimeOfDayInterval& extInterval) const {
        return Contains(Start, Finish, extInterval.Start, extInterval.Finish);
    }

    bool Contains(const TInstant instant) const {
        return Contains(Start, Finish, instant);
    }

    const TTimeOfDay& GetFrom() const {
        return Start;
    }

    const TTimeOfDay& GetTo() const {
        return Finish;
    }

    TTimeOfDayInterval(const TTimeOfDay& start, const TTimeOfDay& finish)
        : Start(start)
        , Finish(finish) {
    }

    TTimeOfDayInterval(const TTimeOfDay& start, const TTimeOfDay& finish, const TTimeZoneHelper timeZone)
        : Start(start.GetValueNoTimezone(), timeZone)
        , Finish(finish.GetValueNoTimezone(), timeZone) {
    }

    TTimeOfDayInterval(const TTimeOfDayInterval& interval, const TTimeZoneHelper timeZone)
        : TTimeOfDayInterval(interval.GetStart(), interval.GetFinish(), timeZone) {
    }

    void DoUnite(const TTimeOfDay from, const TTimeOfDay to) {
        Start = Min(Start, from);
        Finish = Max(Finish, to);
    }

    TTimeInterval GetInstant(const TInstant currentDay) const {
        const TInstant start = Start.GetInstant(currentDay);
        const TInstant finish = Finish.GetInstant(currentDay);
        if (finish < start) {
            return TTimeInterval(start, finish + TDuration::Days(1));
        } else {
            return TTimeInterval(start, finish);
        }
    }

    TString ToString(const bool withTimeZone = false, const TString& separator = "/") const {
        return Start.SerializeToString(withTimeZone) + separator + Finish.SerializeToString(withTimeZone);
    }

    TString SerializeToString(const bool withTimeZone = false, const TString& separator = "/") const {
        return ToString(withTimeZone, separator);
    }

    Y_WARN_UNUSED_RESULT bool DeserializeFromString(const TString& info, const char splitChar = '/') {
        TStringBuf sb(info.data(), info.size());
        return DeserializeFromString(sb, splitChar);
    }

    bool DeserializeFromString(const TStringBuf& info, const char splitChar = '/');
};
