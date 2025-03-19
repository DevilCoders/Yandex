#include "pool.h"

#include <library/cpp/logger/global/global.h>
#include <util/string/strip.h>

TTimeRestrictionsPool::TProto TTimeRestrictionsPool::SerializeToProto() const {
    TProto result;
    for (auto&& i : Restrictions) {
        *result.AddRestrictions() = i.SerializeToProto();
    }
    return result;
}

bool TTimeRestrictionsPool::DeserializeFromProto(const TProto& info) {
    i32 timezone = 0;
    for (auto&& i : info.GetRestrictions()) {
        TTimeRestriction restriction;
        if (!restriction.DeserializeFromProto(i)) {
            return false;
        }
        timezone = restriction.GetTimezoneShift();
        Restrictions.emplace_back(std::move(restriction));
    }
    SetTimezoneShift(timezone);
    return true;
}

const TTimeRestriction* TTimeRestrictionsPool::GetWidestRestriction(const TInstant start, const TInstant finish) const {
    TDuration maxCross = TDuration::Zero();
    const TTimeRestriction* result = nullptr;
    for (auto&& i : Restrictions) {
        const TDuration currentCross = i.GetCrossSize(start, finish);
        if (maxCross < currentCross) {
            maxCross = currentCross;
            result = &i;
        }
    }
    return result;
}

NJson::TJsonValue TTimeRestrictionsPool::SerializeToJson() const {
    NJson::TJsonValue result = NJson::JSON_MAP;
    if (!Restrictions.empty()) {
        result.InsertValue("time_zone", Restrictions.front().GetTimezoneShift());
        NJson::TJsonValue restrictions = NJson::JSON_ARRAY;
        for (auto&& i : Restrictions) {
            restrictions.AppendValue(i.SerializeToJson(false));
        }
        result.InsertValue("restrictions", restrictions);
    }
    return result;
}

bool TTimeRestrictionsPool::DeserializeFromJson(const NJson::TJsonValue& info) {
    if (!info.IsMap()) {
        return false;
    }
    i16 timezone = 0;
    JREAD_INT_OPT(info, "time_zone", timezone);
    NJson::TJsonValue::TArray arrJson;
    if (!info.Has("restrictions") || !info["restrictions"].GetArray(&arrJson)) {
        TFLEventLog::Log("no_restrictions_field")("field_name", "restrictions");
        return false;
    }
    for (auto&& i : arrJson) {
        TTimeRestriction restriction;
        if (!restriction.DeserializeFromJson(i)) {
            return false;
        }
        Restrictions.emplace_back(std::move(restriction));
    }
    SetTimezoneShift(timezone); // Autocompile
    return true;
}

NFrontend::TScheme TTimeRestrictionsPool::GetScheme() {
    NFrontend::TScheme  scheme;
    scheme.Add<TFSArray>("restrictions", "restrictions").SetElement(TTimeRestriction::GetScheme());
    scheme.Add<TFSNumeric>("time_zone", "time_zone").SetDefault(3);
    return scheme;
}

ui32 TTimeRestrictionsPool::GetDurationToSwitch(const TInstant time) const {
    return (GetNextCorrectionTime(time) - time).Seconds();
}

TDuration TTimeRestrictionsPool::GetCrossSize(const TInstant from, const TInstant to) const {
    TDuration result = TDuration::Zero();
    bool isInternal = IsActualNow(from);
    TInstant current = from;
    while (current < to) {
        const TInstant nextSwitch = Min(to, GetNextCorrectionTime(current, (to - current) + TDuration::Seconds(1)));
        if (isInternal) {
            result += nextSwitch - current;
        }
        current = nextSwitch;
        isInternal = !isInternal;
    }
    return result;
}

TVector<TTimeInterval> TTimeRestrictionsPool::CrossInterval(const TInstant from, const TInstant to, const ui32 maxTicks /*= 40*/) const {
    TVector<TTimeInterval> result;
    bool isInternal = IsActualNow(from);
    TInstant current = from;
    ui32 idxTick = 0;
    while (current < to) {
        const TInstant nextSwitch = Min(to, GetNextCorrectionTime(current, (to - current) + TDuration::Seconds(1)));
        if (isInternal) {
            result.emplace_back(current, nextSwitch);
        }
        current = nextSwitch;
        isInternal = !isInternal;
        if (++idxTick > maxTicks) {
            break;
        }
    }
    return result;
}

TMaybe<TTimeOfDayInterval> TTimeRestrictionsPool::GetDayInterval(const TTimeRestriction::EWeekDay day) const {
    TMaybe<TTimeOfDayInterval> result;
    for (auto&& i : Restrictions) {
        if (i.GetDay() & (ui32)day) {
            if (!result) {
                result = TTimeOfDayInterval(i.GetTimeOfDayFrom(), i.GetTimeOfDayTo());
            } else {
                result->DoUnite(i.GetTimeOfDayFrom(), i.GetTimeOfDayTo());
            }
        }
    }
    return result;
}

TMaybe<TTimeOfDayInterval> TTimeRestrictionsPool::GetDayInterval(const TInstant dayInstant) const {
    TMaybe<TTimeOfDayInterval> result;
    for (auto&& i : Restrictions) {
        if (i.IsSameDay(dayInstant)) {
            if (!result) {
                result = TTimeOfDayInterval(i.GetTimeOfDayFrom(), i.GetTimeOfDayTo());
            } else {
                result->DoUnite(i.GetTimeOfDayFrom(), i.GetTimeOfDayTo());
            }
        }
    }
    return result;
}

bool TTimeRestrictionsPool::IsIntervalsIncluded(const TVector<TTimeInterval>& intervals) const {
    for (auto&& i : intervals) {
        if (!IsIntervalIncluded(i)) {
            return false;
        }
    }
    return true;
}

TInstant TTimeRestrictionsPool::GetNextCorrectionTime(const TInstant time, const TDuration timeout /*= TDuration::Days(10)*/) const {
    TInstant next = TInstant::Max();
    if (Restrictions.empty())
        return next;
    TVector<TRestrictionIterator> current(Restrictions.size());
    bool actualStart = false;
    for (ui32 i = 0; i < Restrictions.size(); ++i) {
        current[i].ActualState = Restrictions[i].IsActualNow(time);
        current[i].Restriction = &Restrictions[i];
        actualStart |= current[i].ActualState;
        current[i].Time = time;
        current[i].TimeNext = Restrictions[i].GetNextSwitching(time);
    }
    bool actualCurrent = false;
    MakeHeap(current.begin(), current.end());
    while (true) {
        actualCurrent = false;
        PopHeap(current.begin(), current.end());
        ui32 changesCount = 0;
        for (auto&& i : current) {
            if (i.TimeNext == current.back().TimeNext) {
                i.Time = i.TimeNext;
                i.ActualState = !i.ActualState;
                ++changesCount;
                i.TimeNext = i.Restriction->GetNextSwitching(i.Time);
            }
            actualCurrent |= i.ActualState;
        }
        if (actualStart != actualCurrent) {
            return current.back().Time;
        }
        if (current.back().Time > time + timeout) {
            return time + timeout;
        }
        if (changesCount > 1) {
            MakeHeap(current.begin(), current.end());
        } else {
            PushHeap(current.begin(), current.end());
        }
    };
    return TInstant::Max();
}

void TTimeRestrictionsPool::SetTimezoneShift(const i16 shift) {
    for (auto&& i : Restrictions) {
        i.SetTimezoneShift(shift);
    }
}

bool TTimeRestrictionsPool::IsActualNow(TInstant timestamp) const {
    for (auto&& i : Restrictions) {
        if (i.IsActualNow(timestamp))
            return true;
    }
    return false;
}
